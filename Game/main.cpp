#include <SFML/Graphics.hpp>

#include "config/IniFile.h"

#include "core/platform/PlatformTypes.h"
#include "input/ActionMap.h"
#include "input/Gamepad.h"
#include "input/InputAction.h"
#include "input/InputControl.h"
#include "input/InputSystem.h"
#include "input/Keyboard.h"
#include "input/Mapping.h"
#include "input/Mouse.h"

#include "scene/CameraComponent.h"
#include "scene/ComponentRegistry.h"
#include "scene/MaterialComponent.h"
#include "scene/ParticleSystemComponent.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "scene/SceneSerializer.h"
#include "scene/ScriptComponent.h"
#include "scene/SourceComponent.h"

#include "ui/UIFactory.h"
#include "ui/UIManager.h"
#include "ui/UIButton.h"
#include "ui/UICheckboxGroup.h"
#include "ui/UIHorizontalBox.h"
#include "ui/UIImage.h"
#include "ui/UILabel.h"
#include "ui/UIScrollView.h"
#include "ui/UISlider.h"
#include "ui/UIVerticalBox.h"

#include "assets/AssetCooker.h"
#include "assets/AssetImporterRegistry.h"
#include "assets/AssetManager.h"
#include "assets/FontAsset.h"
#include "assets/FontCodec.h"
#include "assets/LuaCodec.h"
#include "assets/MusicCodec.h"
#include "assets/ShaderAsset.h"
#include "assets/ShaderCodec.h"
#include "assets/SoundCodec.h"
#include "assets/TextureAsset.h"
#include "assets/TextureCodec.h"
#include "render/PostProcessPipeline.h"

#include "ImageWebP.h" // format module: self-registers WebP decoder + import rule

#include "core/FileSystem.h"
#include "core/Window.h"

#include "gfx/GfxRenderer.h"
#include "gfx/InstanceDrawer.h"

#include "scene/InstancedSpriteComponent.h"
#include "scene/SpriteComponent.h"
#include "resource/SpriteAtlas.h"

#include "utils/MemoryPoolHandler.h"
#include "utils/FrameMemory.h"
#include "utils/EventSystem.h"
#include "utils/MemoryPoolHandler.h"
#include "utils/Random.h"

#include "scripts/ScriptEngine.h"

#include "DemoCook.h"
#include "DemoScene.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace sfmx;

int
main(int argc, char **argv) {
  // Offline cooking entry points (exit without opening a window):
  //   --cook [src] [out]  wrap the media under src into .sfmxasset containers.
  //   --cook-scene        build the demo scene in code and serialize it.
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--cook") == 0) {
      const FileSystemPath srcDir = (i + 1 < argc) ? argv[i + 1] : "Game/resources";
      const FileSystemPath outDir = (i + 2 < argc) ? argv[i + 2] : "Game/assets";
      // The cooker consults the importer registry (extension -> asset type + chunk
      // format). Seed the built-in engine formats; a format module would register
      // its own extension here too (see the AssetImporterRegistry docs).
      AssetImporterRegistry::startUp();
      AssetImporterRegistry::instance().registerBuiltins();
      // TODO: this probably needs to be loaded in runtime or something, for now
      // we are dependent and calling this here. We might want to use LoadPlugin
      // later in the game(?)
      imagewebp::registerModule(); // adds the .webp import rule (decoder skipped: no AssetManager in cook)
      AssetCooker::cookDirectory(srcDir, outDir);
      AssetImporterRegistry::shutDown();
      return 0;
    }
    if (std::strcmp(argv[i], "--cook-scene") == 0) {
      return demo::cookScene();
    }
  }

  // Optional content-root override, applied before any content loads: a launcher
  // or installer can point the game at content that is not next to the exe.
  // Precedence: --content-dir <path> (CLI) > SFMX_CONTENT_ROOT (env) > exe dir.
  {
    FileSystemPath contentOverride;
    for (int i = 1; i + 1 < argc; ++i) {
      if (std::strcmp(argv[i], "--content-dir") == 0) {
        contentOverride = argv[i + 1];
        break;
      }
    }
    if (contentOverride.empty()) {
      if (const char* env = std::getenv("SFMX_CONTENT_ROOT");
          nullptr != env && '\0' != env[0]) {
        contentOverride = env;
      }
    }
    if (!contentOverride.empty()) {
      FileSystem::setContentRoot(contentOverride);
      // Flush now: this is a one-time startup diagnostic worth seeing even if a
      // later step aborts before the buffered stdout is flushed.
      std::cout << "[Content] root override -> " << contentOverride.string()
                << std::endl;
    }
  }

  IniFile config;
  // Content paths are relative to the content root (defaults to the exe dir), so
  // the game finds its content next to the exe regardless of the launch CWD.
  config.loadAll({"config/Engine.ini", "config/Game.ini"});

  const uint32 windowWidth  = config.getUInt("Window", "Width", 800u);
  const uint32 windowHeight = config.getUInt("Window", "Height", 600u);
  const String windowTitle  = config.getString("Window", "Title", "SFMX Game");
  const bool enableVSync    = config.getBool("Window", "VSync", true);

  // The Window module owns the sf::RenderWindow and creates it on start-up.
  WindowCreateInfo windowInfo;
  windowInfo.title  = windowTitle;
  windowInfo.width  = windowWidth;
  windowInfo.height = windowHeight;
  Window::startUp(windowInfo);

  sf::RenderWindow &window = Window::instance().getRenderWindow();
  window.setVerticalSyncEnabled(enableVSync);

  // Right after the window, so the shared shader program it owns is created and
  // destroyed strictly inside the lifetime of the window's GL context.
  GfxRenderer::startUp();
  // Batches instanced sprites; owns GPU buffers, so it lives inside the context
  // too (shut down before GfxRenderer / the window below).
  InstanceDrawer::startUp();

  // Engine modules. Order matters: SceneManager clears its scenes at shutDown
  // (returning pooled nodes/components), so it is torn down before the pools,
  // and the AssetManager whose sf::Textures they reference is torn down last.
  MemoryPoolHandler::startUp(4096);
  FrameMemory::startUp(4u * 1024u * 1024u);  // 4 MB arena, rewound each frame
  InputSystem::startUp();
  PhysicsSystem::startUp();
  ComponentRegistry::startUp();
  SceneManager::startUp();

  demo::registerDemoPools(MemoryPoolHandler::instance());
  demo::registerDemoComponents();

  // Mount the cooked .sfmxasset directory (resolved under the content root; the
  // build's POST_BUILD cooks and stages `assets/` next to the exe). Images resolve
  // by UUID through the AssetManager; audio stays mp3-by-path (streams).
  AssetManager::startUp();
  AssetManager::instance().registerCodec(MakeShared<TextureCodec>());
  AssetManager::instance().registerCodec(MakeShared<ShaderCodec>());
  AssetManager::instance().registerCodec(MakeShared<LuaCodec>());
  AssetManager::instance().registerCodec(MakeShared<SoundCodec>());
  AssetManager::instance().registerCodec(MakeShared<MusicCodec>());
  AssetManager::instance().registerCodec(MakeShared<FontCodec>());
  // WebP support: the module registers an IDecoder<sf::Image> for kWebP (import-rule
  // half is a no-op here — the AssetImporterRegistry isn't started in the runtime path).
  imagewebp::registerModule();
#if USING(SFMX_DEBUG_MODE)
  // Dev: load Lua scripts from their raw source (hot-reloadable via F5) instead of the
  // cooked chunk. Debug-only — this block is compiled out of release, so the ini flag
  // is a harmless no-op there. Set before mount/load so the first script load is raw.
  AssetManager::instance().setRawScriptMode(
      config.getBool("Debug", "RawScripts", true),
      config.getString("Debug", "RawSourceDir", "resources"));
#endif
  const size_t mountedAssets = AssetManager::instance().mount("assets");
  std::cout << "[Assets] mounted " << mountedAssets << " from assets\n";

  // UI before ScriptEngine: ScriptEngine::onStartUp binds the Lua globals, and
  // the `UI` global only binds when UIManager is already up (registerUIManager
  // guards on UIManager::isStarted()). Start-up also registers the widget
  // pools, so it must precede the first ui::createWidget below.
  UIManager::startUp();

  ScriptEngine::startUp();

  // Load the cooked demo scene into a SceneManager-owned scene; fall back to
  // building it in code (dev convenience if `--cook-scene` has not run yet).
  SceneManager &scenes = SceneManager::instance();
  Scene* scenePtr = scenes.loadScene("Main", demo::kSceneFile);
  if (nullptr == scenePtr) {
    std::cerr << "[Scene] could not load " << demo::kSceneFile
              << " (run `Game --cook-scene`); building in code\n";
    scenePtr = scenes.createScene("Main");
    demo::buildDemoScene(*scenePtr, static_cast<float>(windowWidth),
                         static_cast<float>(windowHeight));
  }
  Scene &scene = *scenePtr;

  // Full-screen post-processing: the scene is rendered offscreen and run through the
  // cooked post shaders. Held in an Optional so its GL render targets (and the shader
  // they keep alive) are released before the window's context is torn down.
  Optional<PostProcessPipeline> postFx;
  postFx.emplace();
  if (postFx->init(window.getSize())) {
    // CRT pass authored as a .shader manifest: a custom vertex stage + fragment stage,
    // cooked into one multi-chunk asset (exercises the vertex-shader path end to end).
    if (SPtr<ShaderAsset> crt = AssetManager::instance().load<ShaderAsset>(
            sfmx::UUID::createFromName("shaders/crt.shader"))) {
      postFx->addPass(std::move(crt));
    }
  }

  // Wire the behavior the serialized scene does not carry (active camera,
  // music/animation playback, the refs the game loop drives).
  demo::DemoRuntime rt = demo::wireDemoRuntime(scene);

  // InputSystem: "Mapping Mode" demo - a Mapping holds an ActionMap, which holds
  // Actions, each with bindings + an Interaction (tap/hold) and Processors.
  // Jump (tap), Crouch (hold), Move (normalized Vector2).
  Mapping* controls = InputSystem::instance().createMapping("DefaultControls");

  // ── UI ActionMap: keyboard/gamepad navigation ──────────────────────────
  ActionMap* uiActions = controls->addMap("UI");

  InputAction* uiNavigate = uiActions->addAction("Navigate", ActionValueType::kAxis2D);
  CompositeBinding &navComposite = uiNavigate->addComposite(CompositeType::kVector2D);
  navComposite.m_parts.push_back({
    InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kUp), -1, false}, CompositeRole::kNegativeY, {}});
  navComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kDown), -1, false}, CompositeRole::kPositiveY, {}});
  navComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kLeft), -1, false}, CompositeRole::kNegativeX, {}});
  navComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kRight), -1, false}, CompositeRole::kPositiveX, {}});
  navComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kW), -1, false}, CompositeRole::kNegativeY, {}});
  navComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kS), -1, false}, CompositeRole::kPositiveY, {}});
  navComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kA), -1, false}, CompositeRole::kNegativeX, {}});
  navComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kD), -1, false}, CompositeRole::kPositiveX, {}});

  InputAction* uiSubmit = uiActions->addAction("Submit", ActionValueType::kButton);
  uiSubmit->addBinding(InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kSpace), -1, false});
  uiSubmit->addBinding(InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kEnter), -1, false});
  uiSubmit->setInteraction(Interaction{InteractionType::kPress, 0.f});

  InputAction* uiCancel =
      uiActions->addAction("Cancel", ActionValueType::kButton);
  uiCancel->addBinding(InputControl{
      DeviceType::kKeyboard, static_cast<int32>(Key::kEscape), -1, false});
  uiCancel->setInteraction(Interaction{InteractionType::kPress, 0.f});

  InputSystem::instance().setActiveMapping(controls);

  /****************************************************************************/
  /*                                 UI Setup                                 */
  /*                                                                          */

  // Wire up UI navigation actions. No scene-side canvas exists anymore: the
  // UIManager owns the widget roots (started with the engine modules above).
  UIManager& uiManager = UIManager::instance();
  uiManager.setNavigateAction(uiNavigate);
  uiManager.setSubmitAction(uiSubmit);
  uiManager.setCancelAction(uiCancel);

  UILabel* debugLabel = nullptr;
  // Kept alive for the whole loop so the toggle button stays subscribed.
  HEvent toggleShaderHandle;
  UILabel* shaderLabel = nullptr;
  {
    // Load fonts
    SPtr<FontAsset> fontAsset;
    constexpr const char* fontPaths[] = {
      "PlayArea.otf",
    };

    bool fontLoaded = false;

    for (const char* fp : fontPaths) {
      fontAsset = AssetManager::instance().load<FontAsset>(
          sfmx::UUID::createFromName(String(fp)));
      if (fontAsset && fontAsset->isLoaded()) {
        fontLoaded = true;
        break;
      }
    }

    // Debug label
    if (fontLoaded) {
      debugLabel = ui::createWidget<UILabel>(sf::Vector2f{float(windowWidth), 50.f});
      debugLabel->setName("DebugLabel");
      debugLabel->setPosition({25.0f, windowHeight - 50.0f});
      // Bottom-left pinned: authoring position first, anchors next, then the
      // anchor-relative offset LAST (setPosition rewrites the slot offset).
      debugLabel->setAnchorMin({0.f, 1.f});
      debugLabel->setAnchorMax({0.f, 1.f});
      debugLabel->setPivot({0.f, 1.f});
      debugLabel->setOffset({25.0f, 0.f});  // 25 from left, flush to bottom
      debugLabel->setFontAsset(fontAsset);
      debugLabel->setText("");
      debugLabel->setCharacterSize(22);
      debugLabel->setTextColor(sf::Color::White);
      uiManager.addRoot(debugLabel);
    }

    // Show upgrades menu button
    UIButton* upgradesBtn = ui::createWidget<UIButton>(sf::Vector2f{200.f, 50.f});
    upgradesBtn->setName("UpgradesButton");
    upgradesBtn->setPosition({25.0f, 25.0f});
    uiManager.addRoot(upgradesBtn);

    // Info label
    if (fontLoaded) {
      auto* infoLabel = ui::createWidget<UILabel>(sf::Vector2f{400.f, 50.f});
      infoLabel->setName("InfoLabel");
      infoLabel->setPosition({250.0f, 25.0f});
      infoLabel->setFontAsset(fontAsset);
      infoLabel->setText("");
      infoLabel->setCharacterSize(22);
      infoLabel->setTextColor(sf::Color::White);
      uiManager.addRoot(infoLabel);
    }

    // Upgrades menu
    if (fontLoaded) {
      // Upgrades scroll view
      UIScrollView* scrollView = ui::createWidget<UIScrollView>(sf::Vector2f{310.0f, 250.f});
      scrollView->setName("UpgradesMenu");
      scrollView->setPosition({25.0f, 100.0f});
      scrollView->setBackgroundColor(sf::Color(255, 101, 224, 128));
      uiManager.addRoot(scrollView);

      // Upgrades list container
      UIVerticalBox* list = ui::createWidget<UIVerticalBox>(sf::Vector2f{310.f, 60.f});
      list->setName("UpgradesList");
      list->setPadding({15.0f, 10.0f});
      list->setSpacing(5.0f);
      list->setBoxColor(sf::Color::Transparent);
      scrollView->addChild(list);

      // Helper local function to add upgrade entries
      auto addBuyUnitButton = [&](const char* name) {
        // Upgrade container
        UIHorizontalBox* hbox = ui::createWidget<UIHorizontalBox>(sf::Vector2f{280.f, 50.f});
        hbox->setName(String(name) + " HBox");
        hbox->setPosition({0.0f, 0.0f});
        hbox->setPadding({10.0f, 10.0f});
        hbox->setSpacing(10.f);
        hbox->setBoxColor(sf::Color(40, 40, 55, 200));
        list->addChild(hbox);

        // Upgrade name label
        auto* nameLbl = ui::createWidget<UILabel>(sf::Vector2f{150.f, 30.f});
        nameLbl->setName(String(name) + " Label");
        nameLbl->setPosition({0.f, 0.f});
        nameLbl->setFontAsset(fontAsset);
        nameLbl->setText(name);
        nameLbl->setCharacterSize(13);
        nameLbl->setTextColor(sf::Color::White);
        hbox->addChild(nameLbl);

        // Upgrade cost label
        auto* costLbl = ui::createWidget<UILabel>(sf::Vector2f{40.f, 30.f});
        costLbl->setName(String(name) + " Cost Label");
        costLbl->setPosition({0.f, 0.f});
        costLbl->setFontAsset(fontAsset);
        costLbl->setText("$");
        costLbl->setCharacterSize(13);
        costLbl->setTextColor(sf::Color::White);
        hbox->addChild(costLbl);

        // Upgrade button
        auto* btn = ui::createWidget<UIButton>(sf::Vector2f{50.f, 30.f});
        btn->setName(String(name) + " Button");
        btn->setPosition({0.f, 0.f});
        hbox->addChild(btn);

        hbox->updateLayout();
      };

      // Buy quantity slider
      {
        // Buy label
        auto* label = ui::createWidget<UILabel>(sf::Vector2f{180.f, 22.f});
        label->setName("BuyLabel");
        label->setPosition({0.f, 0.f});
        label->setFontAsset(fontAsset);
        label->setText("Amount of units to buy");
        label->setCharacterSize(14);
        label->setTextColor(sf::Color::White);
        list->addChild(label);

        // Buy slider
        UISlider* buySlider = ui::createWidget<UISlider>(sf::Vector2f{180.f, 20.f});
        buySlider->setName("BuySlider");
        buySlider->setPosition({0.f, 0.f});
        buySlider->setRange(1.f, 10.f);
        buySlider->setValue(1.f);
        buySlider->setStepValue(1.0f);
        list->addChild(buySlider);
      }

      addBuyUnitButton("Common Mage");
      addBuyUnitButton("Fire Mage");
      addBuyUnitButton("Thunder Mage");
      addBuyUnitButton("Elder Wizard");
      addBuyUnitButton("Elite Warlock");

      list->updateLayout();

      // Fit the box to content height, scroll view handles overflow
      float contentH = 8.f; // top padding
      for (auto* child : list->getChildren()) {
        contentH += child->getSize().y + 6.f;
      }
      list->setSize({list->getSize().x, contentH});
      scrollView->setContentHeight(contentH);
    }

    // Exit game button — bottom-right pinned (authoring position → anchors →
    // anchor-relative offset LAST: setPosition rewrites the slot offset).
    UIButton* btnExit = ui::createWidget<UIButton>(sf::Vector2f{200.f, 50.f});
    btnExit->setName("ExitBtn");
    btnExit->setPosition({windowWidth - 225.0f, windowHeight - 75.0f});
    btnExit->setAnchorMin({1.f, 1.f});
    btnExit->setAnchorMax({1.f, 1.f});
    btnExit->setPivot({1.f, 1.f});
    btnExit->setOffset({-25.0f, -25.0f});  // 25px margin from the corner
    btnExit->setNormalColor(sf::Color(180, 80, 80));
    uiManager.addRoot(btnExit);

    // Toggle post-processing shader on/off, to eyeball the effect.
    UIButton* toggleShaderBtn = ui::createWidget<UIButton>(sf::Vector2f{200.f, 50.f});
    toggleShaderBtn->setName("ToggleShaderBtn");
    toggleShaderBtn->setPosition({windowWidth - 225.0f, windowHeight - 140.0f});
    toggleShaderBtn->setAnchorMin({1.f, 1.f});
    toggleShaderBtn->setAnchorMax({1.f, 1.f});
    toggleShaderBtn->setPivot({1.f, 1.f});
    toggleShaderBtn->setOffset({-25.0f, -90.0f});
    toggleShaderBtn->setNormalColor(sf::Color(80, 140, 180));
    uiManager.addRoot(toggleShaderBtn);

    if (fontLoaded) {
      shaderLabel = ui::createWidget<UILabel>(sf::Vector2f{200.f, 50.f});
      shaderLabel->setName("ShaderLabel");
      shaderLabel->setPosition({windowWidth - 215.0f, windowHeight - 128.0f});
      shaderLabel->setAnchorMin({1.f, 1.f});
      shaderLabel->setAnchorMax({1.f, 1.f});
      shaderLabel->setPivot({1.f, 1.f});
      shaderLabel->setOffset({-15.0f, -78.0f});
      shaderLabel->setFontAsset(fontAsset);
      shaderLabel->setText("Shader: ON");
      shaderLabel->setCharacterSize(18);
      shaderLabel->setTextColor(sf::Color::White);
      uiManager.addRoot(shaderLabel);
    }

    PostProcessPipeline* fx = postFx ? &*postFx : nullptr;
    toggleShaderHandle = toggleShaderBtn->onPointerClick(
      [fx, label = shaderLabel](sf::Vector2f) {
        if (nullptr == fx) {
          return;
        }
        const bool on = !fx->isEnabled();
        fx->setEnabled(on);
        if (nullptr != label) {
          label->setText(on ? "Shader: ON" : "Shader: OFF");
        }
      }
    );
  }

  /*                                                                          */
  /*                                 UI Setup                                 */
  /****************************************************************************/

  SceneNode* gameManager = scene.createNode("GameManager");
  gameManager->addComponent<ScriptComponent>(sfmx::UUID::createFromName("gameManager.lua"));

  SceneNode* inputDemo = scene.createNode("InputDemo");
  inputDemo->addComponent<ScriptComponent>(sfmx::UUID::createFromName("inputDemo.lua"));
  sfmx::UUID texID = sfmx::UUID::createFromName(String("NumbersMonospace.png"));

  EmitterConfig sampleConfig;
  sampleConfig.maxParticles = 1024 * 100;
  sampleConfig.positionOffset = {0.0f, 0.0f};
  // +Y points down in SFML, so a positive-Y gravity brakes the upward launch.
  sampleConfig.gravity = {0.f, 200.f};
  sampleConfig.startSize = {25.f, 25.f};
  sampleConfig.endSize = {0.f, 0.f};
  // Left null on purpose: the component resolves it from textureAssetId and
  // holds the asset alive for as long as the emitter needs it.
  sampleConfig.texture = nullptr;
  sampleConfig.textureAssetId = texID;
  sampleConfig.blendMode = sf::BlendAlpha;
  sampleConfig.emissionRate = 0.0f;
  sampleConfig.positionVariance = 0.0f;
  // -90 degrees is straight up; the variance fans the jet out a little.
  sampleConfig.direction = sf::degrees(-90.0f);
  sampleConfig.directionVariance = sf::degrees(45.0f);
  sampleConfig.speed = 400.0f;
  sampleConfig.speedVariance = 40.0f;
  sampleConfig.startRotation = sf::Angle::Zero;
  sampleConfig.startRotationVariance = sf::Angle::Zero;
  // Radians per second: a lazy tumble so the stars do not look stamped on.
  sampleConfig.angularVelocity = 0.0f;
  sampleConfig.angularVelocityVariance = 0.0f;
  sampleConfig.startColor = sf::Color::White;
  // Ending on alpha 0 is what makes them disappear rather than pop out.
  sampleConfig.endColor = sf::Color(255, 255, 255, 0);
  // Roughly the time it takes gravity to cancel the launch speed, so they fade
  // out around the top of their arc instead of raining back down.
  sampleConfig.lifetime = 3.0f;
  sampleConfig.lifetimeVariance = 0.25f;
  sampleConfig.duration = 0.f;
  sampleConfig.loop = true;
  // Payload every rate-spawned particle carries. Distinct from the ids the game
  // loop emits by hand, so the two are told apart by colour in the debug
  // shader.
  sampleConfig.customData.id = 67;

  // Sit the emitter near the bottom of whatever the active camera is looking
  // at, so it stays on screen wherever the serialized camera happens to be
  // placed.
  sf::Vector2f emitterPos{0.0f, 0.0f};

  SceneNode* particlesNode = scene.createNode("NumberParticles");
  particlesNode->transform().setPosition(emitterPos);

  auto* particleSystem = particlesNode->addComponent<ParticleSystemComponent>(sampleConfig);
  particleSystem->start();

  EmitterConfig customConfig = particleSystem->getConfig();

  // Per-particle custom data is only observable through a material: the
  // built-in quad program declares no custom-data block, so the renderer skips
  // the upload for it. This debug shader hues each particle by its payload id.
  if (SPtr<ShaderAsset> particleShader =
          AssetManager::instance().load<ShaderAsset>(
              sfmx::UUID::createFromName("shaders/particleCustom.shader"))) {
    auto* particleMaterial = particlesNode->addComponent<MaterialComponent>();
    particleMaterial->setShader(std::move(particleShader));
    particleSystem->setMaterial(particleMaterial);
  } else {
    std::cerr << "[Particles] shaders/particleCustom.shader missing; particles "
                 "will draw with the built-in program and no custom data\n";
  }

  // Instancing stress test: two grids of the same sprite over the same atlas,
  // one drawn through the InstanceDrawer (1 draw call per atlas) and one drawn
  // the classic way (1 draw call per sprite). Only one grid is visible at a
  // time; press B to flip between them and compare the HUD's FPS.
  SceneNode* stressInstanced = nullptr;
  SceneNode* stressRegular = nullptr;
  size_t stressCount = 0;  // sprites per grid (== regular-mode draw calls)
  if (SPtr<TextureAsset> atlas =
          AssetManager::instance().load<TextureAsset>(texID)) {
    const Vector<sf::IntRect> frames =
        sfmx::Atlas::getSpriteRectsByGrid(atlas->texture().getSize(), 10u, 1u);

    constexpr uint32 kStressCols = 300u;
    constexpr uint32 kStressRows = 150u;  // 45k sprites per grid
    constexpr float kStressSpacing = 4.f;
    constexpr float kStressSize = 3.5f;
    stressCount = static_cast<size_t>(kStressCols) * kStressRows;
    const float originX = -static_cast<float>(kStressCols) * kStressSpacing * 0.5f;
    const float originY = -static_cast<float>(kStressRows) * kStressSpacing * 0.5f;

    stressInstanced = scene.createNode("StressInstanced");
    stressRegular = scene.createNode("StressRegular");
    stressRegular->setVisible(false);  // start on the instanced grid

    for (uint32 r = 0; r < kStressRows; ++r) {
      for (uint32 c = 0; c < kStressCols; ++c) {
        const sf::Vector2f pos = {originX + static_cast<float>(c) * kStressSpacing,
                                  originY + static_cast<float>(r) * kStressSpacing};
        const uint32 frame = (r * kStressCols + c) % 10u;

        SceneNode* inst = stressInstanced->createChild("i");
        inst->transform().setPosition(pos);
        auto* isprite = inst->addComponent<InstancedSpriteComponent>();
        isprite->setAtlas(atlas, frames, sf::BlendAlpha, kStressCols * kStressRows);
        isprite->setFrame(frame);
        isprite->setSize({kStressSize, kStressSize});

        SceneNode* reg = stressRegular->createChild("r");
        reg->transform().setPosition(pos);
        auto* rsprite = reg->addComponent<SpriteComponent>();
        rsprite->setTextureAsset(atlas);
        rsprite->setRect(frames[frame]);
        rsprite->setScale({kStressSize / static_cast<float>(frames[frame].size.x),
                           kStressSize / static_cast<float>(frames[frame].size.y)});
      }
    }
  }

  sf::Clock clock;

  bool showInstanced = true;  // which stress grid is currently visible
  bool vsyncOn = enableVSync; // toggled with V to uncap the frame rate

  constexpr size_t deltasSize = 100;
  std::array<float, deltasSize> deltas;
  uint32 index = 0;
  float totalTime = 0.0f;

  while (window.isOpen()) {
    // InputSystem: snapshot device state before polling
    InputSystem::instance().beginFrame();

    while (const Optional<sf::Event> event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
        InputSystem::instance().onEvent(*event);
      }
      else if (UIManager::instance().handleEvent(*event)) {
        // UI consumed it: a focused text editor owns typing, caret keys and
        // Escape-to-cancel — the game input layer must not see those strokes.
      }
      else {
        InputSystem::instance().onEvent(*event);
      }
    }

    const float deltaTime = clock.restart().asSeconds();

    deltas[index] = deltaTime;
    index = (index + 1) % deltasSize;
    float avg = 0.0f;
    for (uint32 i = 0; i < deltasSize; ++i) {
      avg += deltas[index];
    }
    avg /= static_cast<float>(deltasSize);
    
    // The HUD label is written every frame; build the transient text in the
    // frame arena instead of std::format's heap string. setText copies the data,
    // so the buffer only needs to live until the call returns.
    char* textBuffer = static_cast<char*>(FrameMemory::instance().allocate(160));
    if (nullptr != textBuffer) {
      const size_t drawCalls =
          showInstanced ? InstanceDrawer::instance().getLastDrawCalls()
                        : stressCount;
      std::snprintf(textBuffer, 160,
                    "FPS: %.0f\nNodes: %zu\nStress[B]: %s\nDraws: %zu\nVSync[V]: %s",
                    std::round(1.0f / avg), scene.getNodeCount(),
                    showInstanced ? "INSTANCED" : "REGULAR",
                    drawCalls, vsyncOn ? "on" : "off");
      debugLabel->setText(StringView(textBuffer));
    }
    
    InputSystem::instance().update(deltaTime, window);

    if (Keyboard::instance().wasPressedThisFrame(Key::kEscape)) {
      window.close();
    }

    if (Keyboard::instance().wasPressedThisFrame(Key::kI)) {
      std::cout << "Current particles: "
                << particleSystem->getParticleCount()
                << std::endl;
    }

    if (Keyboard::instance().wasPressedThisFrame(Key::kB) &&
        nullptr != stressInstanced && nullptr != stressRegular) {
      showInstanced = !showInstanced;
      stressInstanced->setVisible(showInstanced);
      stressRegular->setVisible(!showInstanced);
      std::cout << "[Stress] mode: "
                << (showInstanced ? "INSTANCED" : "REGULAR")
                << " | sprites/grid: " << stressCount
                << " | FPS: " << std::round(1.0f / avg) << std::endl;
    }

    if (Keyboard::instance().wasPressedThisFrame(Key::kV)) {
      vsyncOn = !vsyncOn;
      window.setVerticalSyncEnabled(vsyncOn);
      std::cout << "[Stress] vsync: " << (vsyncOn ? "on" : "off") << std::endl;
    }

#if USING(SFMX_DEBUG_MODE)
    // Dev hot-reload: F5 re-decodes each script's LuaAsset (its raw source in
    // raw mode) and re-binds it, so an edited .lua takes effect without
    // restarting the game.
    if (AssetManager::instance().getRawScriptMode() &&
        Keyboard::instance().wasPressedThisFrame(Key::kF5)) {
      scene.forEachNode([](SceneNode* n) {
        if (auto* sc = n->getComponent<ScriptComponent>()) {
          const sfmx::UUID id = sc->getScriptAssetId();
          if (id != sfmx::UUID::null()) {
            static_cast<void>(
                AssetManager::instance().reload(id)); // re-decode (raw re-read)
            sc->setScriptAssetId(id);                 // re-bind (recompile)
          }
        }
      });
      std::cout << "[Script] hot-reloaded (F5)\n";
    }
#endif

    // Finalize any assets whose background decode completed (GPU upload on
    // this, the GL-owning thread) and fire their loadAsync callbacks BEFORE the
    // scene updates, so components/scripts see freshly loaded assets this same
    // frame.
    AssetManager::instance().finalize();

    UIManager::instance().update(window, deltaTime);

    SceneManager::instance().update(deltaTime);

    totalTime += deltaTime;
    window.clear(sf::Color(24, 24, 28));

    // Scene through the post chain (or straight to the window when no passes).
    postFx->render(scenes, window, totalTime);

    // Screen-space UI, drawn AFTER the post chain so the HUD stays crisp.
    // UIManager::draw switches to the default view itself (window pixels,
    // matching the hit-test coordinate space) and restores it afterwards.
    UIManager::instance().draw(window);

    window.display();

    // End of frame: reclaim every FrameMemory allocation made this frame.
    FrameMemory::instance().endFrame();
  }

  // Release any pending async-load callbacks (they may hold Lua closures) and
  // tear down the scenes (their ScriptComponents hold Lua handles) while the
  // script engine / Lua state is still alive — ScriptEngine / AssetManager shut
  // down just below.
  AssetManager::instance().cancelAsyncLoads();
  SceneManager::instance().destroyAllScenes();

  // Destroy the widget roots after the scenes (their ScriptComponents have
  // unsubscribed from widget events by now) but before ScriptEngine shuts the
  // Lua state down and before the pools go away.
  UIManager::shutDown();

  ScriptEngine::shutDown();
  AssetManager::shutDown();
  // Shut the scene manager down before the pools: it clears every scene, which
  // returns pooled nodes/components while the pools (and SFML) are still alive.
  SceneManager::shutDown();
  ComponentRegistry::shutDown();

  PhysicsSystem::shutDown();
  InputSystem::shutDown();
  FrameMemory::shutDown();
  MemoryPoolHandler::shutDown();

  postFx.reset();
  InstanceDrawer::shutDown();
  GfxRenderer::shutDown();
  Window::shutDown();

  return 0;
}
