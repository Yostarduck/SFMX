#include "Game.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <SFML/Graphics.hpp>

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
#include "core/FileSystem.h"
#include "core/Window.h"
#include "core/physics/PhysicsSystem.h"
#include "gfx/GfxRenderer.h"
#include "gfx/InstanceDrawer.h"
#include "input/ActionMap.h"
#include "input/InputAction.h"
#include "input/InputControl.h"
#include "input/InputSystem.h"
#include "input/Keyboard.h"
#include "input/Mapping.h"
#include "resource/SpriteAtlas.h"
#include "scene/ComponentRegistry.h"
#include "scene/InstancedSpriteComponent.h"
#include "scene/MaterialComponent.h"
#include "scene/ParticleSystemComponent.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "scene/ScriptComponent.h"
#include "scene/SpriteComponent.h"
#include "scripts/ScriptEngine.h"
#include "ui/UIButton.h"
#include "ui/UIFactory.h"
#include "ui/UIHorizontalBox.h"
#include "ui/UILabel.h"
#include "ui/UIManager.h"
#include "ui/UIScrollView.h"
#include "ui/UISlider.h"
#include "ui/UIVerticalBox.h"
#include "utils/FrameMemory.h"
#include "utils/MemoryPoolHandler.h"
#include "utils/UUID.h"

#include "DemoCook.h"
#include "DemoScene.h"
#include "ImageWebP.h" // format module: self-registers WebP decoder + import rule

namespace sfmx
{

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void
Game::initialize(int argc, char **argv) {
  m_argParser.parse(argc, argv);

  // Offline commands: cook content and exit without creating a window.
  if (m_argParser.hasFlag("cook")) {
    m_exitCode = cook() ? 0 : 1;
    m_shouldExit = true;
    return;
  }
  if (m_argParser.hasFlag("cook-scene")) {
    m_exitCode = cookGame() ? 0 : 1;
    m_shouldExit = true;
    return;
  }

  applyContentRootOverride();

  // Content paths are relative to the content root (defaults to the exe dir).
  m_config.loadAll({"config/Engine.ini", "config/Game.ini"});
  m_windowWidth  = m_config.getUInt("Window", "Width", 800u);
  m_windowHeight = m_config.getUInt("Window", "Height", 600u);
  m_vsyncOn      = m_config.getBool("Window", "VSync", true);

  initWindow();
  initModules();
  initAssets();
  initScriptingAndUI();
  initScene();
  initPostProcessing();
  initInput();
  initUI();
  initDemoContent();

  m_clock.restart();
  m_initialized = true;
}

void
Game::pollEvents() {
  // InputSystem: snapshot device state before polling.
  InputSystem::instance().beginFrame();

  sf::RenderWindow &window = Window::instance().getRenderWindow();
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
}

void
Game::update(float deltaTime) {
  // Rolling average for the HUD (the previous frame's window is intentional:
  // the newest sample is included, the oldest one falls out).
  m_deltas[m_deltaIndex] = deltaTime;
  m_deltaIndex = (m_deltaIndex + 1) % kDeltaSamples;

  updateHud();

  sf::RenderWindow &window = Window::instance().getRenderWindow();

  InputSystem::instance().update(deltaTime, window);
  handleInput();

  // Finalize assets whose background decode completed before the scene updates,
  // so components/scripts see freshly loaded assets this same frame.
  AssetManager::instance().finalize();

  UIManager::instance().update(window, deltaTime);
  SceneManager::instance().update(deltaTime);

  m_totalTime += deltaTime;
}

void
Game::render() {
  sf::RenderWindow &window = Window::instance().getRenderWindow();

  window.clear(sf::Color(24, 24, 28));

  if (m_postFx) {
    m_postFx->render(SceneManager::instance(), window, m_totalTime);
  }
  else {
    SceneManager::instance().draw(window);
  }

  // Screen-space UI, drawn AFTER the post chain so the HUD stays crisp.
  UIManager::instance().draw(window);

  window.display();

  // End of frame: reclaim every FrameMemory allocation made this frame.
  FrameMemory::instance().endFrame();
}

void
Game::run() {
  sf::RenderWindow &window = Window::instance().getRenderWindow();
  while (window.isOpen()) {
    pollEvents();
    const float deltaTime = m_clock.restart().asSeconds();
    update(deltaTime);
    render();
  }
}

void
Game::shutdown() {
  if (!m_initialized) {
    return; // A cook command never started the engine modules.
  }

  // Disconnect UI listeners before the widgets that own the emitters die.
  m_toggleShaderHandle = HEvent{};
  m_debugLabel = nullptr;
  m_shaderLabel = nullptr;

  AssetManager::instance().cancelAsyncLoads();
  SceneManager::instance().destroyAllScenes();

  // Widgets and scripts must go while their emitters and the Lua state are alive.
  UIManager::shutDown();
  ScriptEngine::shutDown();
  AssetManager::shutDown();
  SceneManager::shutDown();
  ComponentRegistry::shutDown();

  PhysicsSystem::shutDown();
  InputSystem::shutDown();
  FrameMemory::shutDown();
  MemoryPoolHandler::shutDown();

  // GL-backed objects must die inside the window's context.
  m_postFx.reset();
  InstanceDrawer::shutDown();
  GfxRenderer::shutDown();
  Window::shutDown();

  m_initialized = false;
}

// ---------------------------------------------------------------------------
// Command modes
// ---------------------------------------------------------------------------

bool
Game::cook() {
  const Vector<String> flagValues = m_argParser.getFlagValues("cook");
  const FileSystemPath srcDir =
      flagValues.size() > 0 ? flagValues[0] : "Game/resources";
  const FileSystemPath outDir =
      flagValues.size() > 1 ? flagValues[1] : "Game/assets";

  if (!FileSystem::isDirectory(srcDir)) {
    std::cerr << "[Cook] source directory not found: " << srcDir.string()
              << '\n';
    return false;
  }

  // The cooker consults the importer registry (extension -> asset type + chunk
  // format). A format module registers its own extension here too.
  AssetImporterRegistry::startUp();
  AssetImporterRegistry::instance().registerBuiltins();
  imagewebp::registerModule();
  const CookStats stats = AssetCooker::cookDirectory(srcDir, outDir);
  AssetImporterRegistry::shutDown();

  if (0 == stats.cooked) {
    std::cerr << "[Cook] no supported files were cooked from "
              << srcDir.string() << '\n';
    return false;
  }
  return true;
}

bool
Game::cookGame() {
  // Same authoring path the old main used: build the demo scene in code and
  // serialize it to demo::kSceneFile. Media must be cooked first.
  return 0 == demo::cookScene();
}

// ---------------------------------------------------------------------------
// Startup phases
// ---------------------------------------------------------------------------

void
Game::applyContentRootOverride() {
  // Precedence: --content-dir <path> (CLI) > SFMX_CONTENT_ROOT (env) > exe dir.
  FileSystemPath contentOverride;
  if (m_argParser.hasFlag("content-dir")) {
    const String value(m_argParser.getFlagValue("content-dir", 0));
    if (!value.empty()) {
      contentOverride = FileSystemPath(value);
    }
  }
  if (contentOverride.empty()) {
    if (const char *env = std::getenv("SFMX_CONTENT_ROOT");
        nullptr != env && '\0' != env[0]) {
      contentOverride = FileSystemPath(env);
    }
  }
  if (!contentOverride.empty()) {
    FileSystem::setContentRoot(contentOverride);
    std::cout << "[Content] root override -> " << contentOverride.string()
              << std::endl;
  }
}

void
Game::initWindow() {
  const String windowTitle = m_config.getString("Window", "Title", "SFMX Game");

  WindowCreateInfo windowInfo;
  windowInfo.title  = windowTitle;
  windowInfo.width  = m_windowWidth;
  windowInfo.height = m_windowHeight;
  Window::startUp(windowInfo);

  sf::RenderWindow &window = Window::instance().getRenderWindow();
  window.setVerticalSyncEnabled(m_vsyncOn);

  // Right after the window: their GL resources must live inside its context.
  GfxRenderer::startUp();
  InstanceDrawer::startUp();
}

void
Game::initModules() {
  // Order matters: SceneManager clears its scenes at shutDown (returning pooled
  // nodes/components), so it is torn down before the pools.
  MemoryPoolHandler::startUp(4096);
  FrameMemory::startUp(4u * 1024u * 1024u); // 4 MB arena, rewound each frame
  InputSystem::startUp();
  PhysicsSystem::startUp();
  ComponentRegistry::startUp();
  SceneManager::startUp();

  demo::registerDemoPools(MemoryPoolHandler::instance());
  demo::registerDemoComponents();
}

void
Game::initAssets() {
  AssetManager::startUp();
  AssetManager &assets = AssetManager::instance();
  assets.registerCodec(MakeShared<TextureCodec>());
  assets.registerCodec(MakeShared<ShaderCodec>());
  assets.registerCodec(MakeShared<LuaCodec>());
  assets.registerCodec(MakeShared<SoundCodec>());
  assets.registerCodec(MakeShared<MusicCodec>());
  assets.registerCodec(MakeShared<FontCodec>());

  // WebP support: the module registers an IDecoder<sf::Image> for kWebP.
  imagewebp::registerModule();

#if USING(SFMX_DEBUG_MODE)
  // Dev: load Lua scripts from raw source (hot-reloadable with F5) instead of the
  // cooked chunk. Compiled out in release.
  assets.setRawScriptMode(
      m_config.getBool("Debug", "RawScripts", true),
      m_config.getString("Debug", "RawSourceDir", "resources"));
#endif

  const size_t mountedAssets = assets.mount("assets");
  std::cout << "[Assets] mounted " << mountedAssets << " from assets\n";
}

void
Game::initScriptingAndUI() {
  // UI before ScriptEngine: ScriptEngine::onStartUp binds the Lua globals, and
  // the `UI` global only binds when UIManager is already up. Start-up also
  // registers the widget pools, so it must precede the first ui::createWidget.
  UIManager::startUp();
  ScriptEngine::startUp();
}

void
Game::initScene() {
  SceneManager &scenes = SceneManager::instance();
  Scene *scenePtr = scenes.loadScene("Main", demo::kSceneFile);
  if (nullptr == scenePtr) {
    std::cerr << "[Scene] could not load " << demo::kSceneFile
              << " (run `Game --cook-scene`); building in code\n";
    scenePtr = scenes.createScene("Main");
    demo::buildDemoScene(*scenePtr, static_cast<float>(m_windowWidth),
                         static_cast<float>(m_windowHeight));
  }
  m_scene = scenePtr;

  // Wire the behavior the serialized scene does not carry (active camera etc.).
  demo::wireDemoRuntime(*m_scene);
}

void
Game::initPostProcessing() {
  sf::RenderWindow &window = Window::instance().getRenderWindow();

  // Held in an Optional so its GL render targets are released before the
  // window's context is torn down.
  m_postFx.emplace();
  if (m_postFx->init(window.getSize())) {
    // CRT pass authored as a .shader manifest: custom vertex + fragment stages.
    if (SPtr<ShaderAsset> crt = AssetManager::instance().load<ShaderAsset>(
            UUID::createFromName("shaders/crt.shader"))) {
      m_postFx->addPass(std::move(crt));
    }
  }
}

void
Game::initInput() {
  // InputSystem: "Mapping Mode" demo - a Mapping holds an ActionMap, which holds
  // Actions, each with bindings + an Interaction (tap/hold) and Processors.
  Mapping *controls = InputSystem::instance().createMapping("DefaultControls");

  // ── UI ActionMap: keyboard/gamepad navigation ──────────────────────────
  ActionMap *uiActions = controls->addMap("UI");

  InputAction *uiNavigate =
      uiActions->addAction("Navigate", ActionValueType::kAxis2D);
  CompositeBinding &navComposite =
      uiNavigate->addComposite(CompositeType::kVector2D);
  navComposite.m_parts.push_back({InputControl{DeviceType::kKeyboard,
                                               static_cast<int32>(Key::kUp),
                                               -1, false},
                                  CompositeRole::kNegativeY, {}});
  navComposite.m_parts.push_back({InputControl{DeviceType::kKeyboard,
                                               static_cast<int32>(Key::kDown),
                                               -1, false},
                                  CompositeRole::kPositiveY, {}});
  navComposite.m_parts.push_back({InputControl{DeviceType::kKeyboard,
                                               static_cast<int32>(Key::kLeft),
                                               -1, false},
                                  CompositeRole::kNegativeX, {}});
  navComposite.m_parts.push_back({InputControl{DeviceType::kKeyboard,
                                               static_cast<int32>(Key::kRight),
                                               -1, false},
                                  CompositeRole::kPositiveX, {}});
  navComposite.m_parts.push_back({InputControl{DeviceType::kKeyboard,
                                               static_cast<int32>(Key::kW), -1,
                                               false},
                                  CompositeRole::kNegativeY, {}});
  navComposite.m_parts.push_back({InputControl{DeviceType::kKeyboard,
                                               static_cast<int32>(Key::kS), -1,
                                               false},
                                  CompositeRole::kPositiveY, {}});
  navComposite.m_parts.push_back({InputControl{DeviceType::kKeyboard,
                                               static_cast<int32>(Key::kA), -1,
                                               false},
                                  CompositeRole::kNegativeX, {}});
  navComposite.m_parts.push_back({InputControl{DeviceType::kKeyboard,
                                               static_cast<int32>(Key::kD), -1,
                                               false},
                                  CompositeRole::kPositiveX, {}});

  InputAction *uiSubmit = uiActions->addAction("Submit", ActionValueType::kButton);
  uiSubmit->addBinding(
      InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kSpace), -1, false});
  uiSubmit->addBinding(
      InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kEnter), -1, false});
  uiSubmit->setInteraction(Interaction{InteractionType::kPress, 0.f});

  InputAction *uiCancel = uiActions->addAction("Cancel", ActionValueType::kButton);
  uiCancel->addBinding(
      InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kEscape), -1, false});
  uiCancel->setInteraction(Interaction{InteractionType::kPress, 0.f});

  InputSystem::instance().setActiveMapping(controls);

  // Wire the UI navigation actions: UIManager polls/reads them each frame.
  UIManager &uiManager = UIManager::instance();
  uiManager.setNavigateAction(uiNavigate);
  uiManager.setSubmitAction(uiSubmit);
  uiManager.setCancelAction(uiCancel);
}

void
Game::initUI() {
  // No scene-side canvas exists: UIManager owns the widget roots.
  UIManager &uiManager = UIManager::instance();
  const float windowWidth = static_cast<float>(m_windowWidth);
  const float windowHeight = static_cast<float>(m_windowHeight);

  // Load fonts
  SPtr<FontAsset> fontAsset;
  constexpr const char *fontPaths[] = {
      "PlayArea.otf",
  };

  bool fontLoaded = false;
  for (const char *fp : fontPaths) {
    fontAsset = AssetManager::instance().load<FontAsset>(
        UUID::createFromName(String(fp)));
    if (fontAsset && fontAsset->isLoaded()) {
      fontLoaded = true;
      break;
    }
  }

  // Debug label (bottom-left pinned: position first, anchors next, offset LAST).
  if (fontLoaded) {
    m_debugLabel = ui::createWidget<UILabel>(sf::Vector2f{windowWidth, 50.f});
    m_debugLabel->setName("DebugLabel");
    m_debugLabel->setPosition({25.0f, windowHeight - 50.0f});
    m_debugLabel->setAnchorMin({0.f, 1.f});
    m_debugLabel->setAnchorMax({0.f, 1.f});
    m_debugLabel->setPivot({0.f, 1.f});
    m_debugLabel->setOffset({25.0f, 0.f});
    m_debugLabel->setFontAsset(fontAsset);
    m_debugLabel->setText("");
    m_debugLabel->setCharacterSize(22);
    m_debugLabel->setTextColor(sf::Color::White);
    uiManager.addRoot(m_debugLabel);
  }

  // Show upgrades menu button
  UIButton *upgradesBtn = ui::createWidget<UIButton>(sf::Vector2f{200.f, 50.f});
  upgradesBtn->setName("UpgradesButton");
  upgradesBtn->setPosition({25.0f, 25.0f});
  uiManager.addRoot(upgradesBtn);

  // Info label
  if (fontLoaded) {
    auto *infoLabel = ui::createWidget<UILabel>(sf::Vector2f{400.f, 50.f});
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
    UIScrollView *scrollView =
        ui::createWidget<UIScrollView>(sf::Vector2f{310.0f, 250.f});
    scrollView->setName("UpgradesMenu");
    scrollView->setPosition({25.0f, 100.0f});
    scrollView->setBackgroundColor(sf::Color(255, 101, 224, 128));
    uiManager.addRoot(scrollView);

    UIVerticalBox *list = ui::createWidget<UIVerticalBox>(sf::Vector2f{310.f, 60.f});
    list->setName("UpgradesList");
    list->setPadding({15.0f, 10.0f});
    list->setSpacing(5.0f);
    list->setBoxColor(sf::Color::Transparent);
    scrollView->addChild(list);

    auto addBuyUnitButton = [&](const char *name) {
      UIHorizontalBox *hbox =
          ui::createWidget<UIHorizontalBox>(sf::Vector2f{280.f, 50.f});
      hbox->setName(String(name) + " HBox");
      hbox->setPosition({0.0f, 0.0f});
      hbox->setPadding({10.0f, 10.0f});
      hbox->setSpacing(10.f);
      hbox->setBoxColor(sf::Color(40, 40, 55, 200));
      list->addChild(hbox);

      auto *nameLbl = ui::createWidget<UILabel>(sf::Vector2f{150.f, 30.f});
      nameLbl->setName(String(name) + " Label");
      nameLbl->setPosition({0.f, 0.f});
      nameLbl->setFontAsset(fontAsset);
      nameLbl->setText(name);
      nameLbl->setCharacterSize(13);
      nameLbl->setTextColor(sf::Color::White);
      hbox->addChild(nameLbl);

      auto *costLbl = ui::createWidget<UILabel>(sf::Vector2f{40.f, 30.f});
      costLbl->setName(String(name) + " Cost Label");
      costLbl->setPosition({0.f, 0.f});
      costLbl->setFontAsset(fontAsset);
      costLbl->setText("$");
      costLbl->setCharacterSize(13);
      costLbl->setTextColor(sf::Color::White);
      hbox->addChild(costLbl);

      auto *btn = ui::createWidget<UIButton>(sf::Vector2f{50.f, 30.f});
      btn->setName(String(name) + " Button");
      btn->setPosition({0.f, 0.f});
      hbox->addChild(btn);

      hbox->updateLayout();
    };

    // Buy quantity slider
    {
      auto *label = ui::createWidget<UILabel>(sf::Vector2f{180.f, 22.f});
      label->setName("BuyLabel");
      label->setPosition({0.f, 0.f});
      label->setFontAsset(fontAsset);
      label->setText("Amount of units to buy");
      label->setCharacterSize(14);
      label->setTextColor(sf::Color::White);
      list->addChild(label);

      UISlider *buySlider =
          ui::createWidget<UISlider>(sf::Vector2f{180.f, 20.f});
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
    for (auto *child : list->getChildren()) {
      contentH += child->getSize().y + 6.f;
    }
    list->setSize({list->getSize().x, contentH});
    scrollView->setContentHeight(contentH);
  }

  // Exit game button — bottom-right pinned.
  UIButton *btnExit = ui::createWidget<UIButton>(sf::Vector2f{200.f, 50.f});
  btnExit->setName("ExitBtn");
  btnExit->setPosition({windowWidth - 225.0f, windowHeight - 75.0f});
  btnExit->setAnchorMin({1.f, 1.f});
  btnExit->setAnchorMax({1.f, 1.f});
  btnExit->setPivot({1.f, 1.f});
  btnExit->setOffset({-25.0f, -25.0f}); // 25px margin from the corner
  btnExit->setNormalColor(sf::Color(180, 80, 80));
  uiManager.addRoot(btnExit);

  // Toggle post-processing shader on/off, to eyeball the effect.
  UIButton *toggleShaderBtn = ui::createWidget<UIButton>(sf::Vector2f{200.f, 50.f});
  toggleShaderBtn->setName("ToggleShaderBtn");
  toggleShaderBtn->setPosition({windowWidth - 225.0f, windowHeight - 140.0f});
  toggleShaderBtn->setAnchorMin({1.f, 1.f});
  toggleShaderBtn->setAnchorMax({1.f, 1.f});
  toggleShaderBtn->setPivot({1.f, 1.f});
  toggleShaderBtn->setOffset({-25.0f, -90.0f});
  toggleShaderBtn->setNormalColor(sf::Color(80, 140, 180));
  uiManager.addRoot(toggleShaderBtn);

  if (fontLoaded) {
    m_shaderLabel = ui::createWidget<UILabel>(sf::Vector2f{200.f, 50.f});
    m_shaderLabel->setName("ShaderLabel");
    m_shaderLabel->setPosition({windowWidth - 215.0f, windowHeight - 128.0f});
    m_shaderLabel->setAnchorMin({1.f, 1.f});
    m_shaderLabel->setAnchorMax({1.f, 1.f});
    m_shaderLabel->setPivot({1.f, 1.f});
    m_shaderLabel->setOffset({-15.0f, -78.0f});
    m_shaderLabel->setFontAsset(fontAsset);
    m_shaderLabel->setText("Shader: ON");
    m_shaderLabel->setCharacterSize(18);
    m_shaderLabel->setTextColor(sf::Color::White);
    uiManager.addRoot(m_shaderLabel);
  }

  PostProcessPipeline *fx = m_postFx ? &*m_postFx : nullptr;
  m_toggleShaderHandle = toggleShaderBtn->onPointerClick(
      [fx, label = m_shaderLabel](sf::Vector2f) {
        if (nullptr == fx) {
          return;
        }
        const bool on = !fx->isEnabled();
        fx->setEnabled(on);
        if (nullptr != label) {
          label->setText(on ? "Shader: ON" : "Shader: OFF");
        }
      });
}

void
Game::initDemoContent() {
  Scene &scene = *m_scene;

  SceneNode *gameManager = scene.createNode("GameManager");
  gameManager->addComponent<ScriptComponent>(
      UUID::createFromName("gameManager.lua"));

  SceneNode *inputDemo = scene.createNode("InputDemo");
  inputDemo->addComponent<ScriptComponent>(
      UUID::createFromName("inputDemo.lua"));

  const UUID texID = UUID::createFromName(String("NumbersMonospace.png"));

  EmitterConfig sampleConfig;
  sampleConfig.maxParticles = 1024 * 100;
  sampleConfig.positionOffset = {0.0f, 0.0f};
  sampleConfig.gravity = {0.f, 200.f};
  sampleConfig.startSize = {25.f, 25.f};
  sampleConfig.endSize = {0.f, 0.f};
  sampleConfig.texture = nullptr;
  sampleConfig.textureAssetId = texID;
  sampleConfig.blendMode = sf::BlendAlpha;
  sampleConfig.emissionRate = 0.0f;
  sampleConfig.positionVariance = 0.0f;
  sampleConfig.direction = sf::degrees(-90.0f);
  sampleConfig.directionVariance = sf::degrees(45.0f);
  sampleConfig.speed = 400.0f;
  sampleConfig.speedVariance = 40.0f;
  sampleConfig.startRotation = sf::Angle::Zero;
  sampleConfig.startRotationVariance = sf::Angle::Zero;
  sampleConfig.angularVelocity = 0.0f;
  sampleConfig.angularVelocityVariance = 0.0f;
  sampleConfig.startColor = sf::Color::White;
  sampleConfig.endColor = sf::Color(255, 255, 255, 0);
  sampleConfig.lifetime = 3.0f;
  sampleConfig.lifetimeVariance = 0.25f;
  sampleConfig.duration = 0.f;
  sampleConfig.loop = true;
  sampleConfig.customData.id = 67;

  SceneNode *particlesNode = scene.createNode("NumberParticles");
  particlesNode->transform().setPosition({0.0f, 0.0f});

  m_particleSystem =
      particlesNode->addComponent<ParticleSystemComponent>(sampleConfig);
  m_particleSystem->start();

  // Per-particle custom data is only observable through a material.
  if (SPtr<ShaderAsset> particleShader =
          AssetManager::instance().load<ShaderAsset>(
              UUID::createFromName("shaders/particleCustom.shader"))) {
    auto *particleMaterial = particlesNode->addComponent<MaterialComponent>();
    particleMaterial->setShader(std::move(particleShader));
    m_particleSystem->setMaterial(particleMaterial);
  }
  else {
    std::cerr << "[Particles] shaders/particleCustom.shader missing; particles "
                 "will draw with the built-in program and no custom data\n";
  }

  // Instancing stress test: two grids of the same sprite over the same atlas.
  if (SPtr<TextureAsset> atlas =
          AssetManager::instance().load<TextureAsset>(texID)) {
    const Vector<sf::IntRect> frames =
        sfmx::Atlas::getSpriteRectsByGrid(atlas->texture().getSize(), 10u, 1u);

    constexpr uint32 kStressCols = 300u;
    constexpr uint32 kStressRows = 150u; // 45k sprites per grid
    constexpr float kStressSpacing = 4.f;
    constexpr float kStressSize = 3.5f;
    m_stressCount = static_cast<size_t>(kStressCols) * kStressRows;
    const float originX =
        -static_cast<float>(kStressCols) * kStressSpacing * 0.5f;
    const float originY =
        -static_cast<float>(kStressRows) * kStressSpacing * 0.5f;

    m_stressInstanced = scene.createNode("StressInstanced");
    m_stressRegular = scene.createNode("StressRegular");
    m_stressRegular->setVisible(false); // start on the instanced grid

    for (uint32 r = 0; r < kStressRows; ++r) {
      for (uint32 c = 0; c < kStressCols; ++c) {
        const sf::Vector2f pos = {
            originX + static_cast<float>(c) * kStressSpacing,
            originY + static_cast<float>(r) * kStressSpacing};
        const uint32 frame = (r * kStressCols + c) % 10u;

        SceneNode *inst = m_stressInstanced->createChild("i");
        inst->transform().setPosition(pos);
        auto *isprite = inst->addComponent<InstancedSpriteComponent>();
        isprite->setAtlas(atlas, frames, sf::BlendAlpha,
                          kStressCols * kStressRows);
        isprite->setFrame(frame);
        isprite->setSize({kStressSize, kStressSize});

        SceneNode *reg = m_stressRegular->createChild("r");
        reg->transform().setPosition(pos);
        auto *rsprite = reg->addComponent<SpriteComponent>();
        rsprite->setTextureAsset(atlas);
        rsprite->setRect(frames[frame]);
        rsprite->setScale(
            {kStressSize / static_cast<float>(frames[frame].size.x),
             kStressSize / static_cast<float>(frames[frame].size.y)});
      }
    }
  }

  m_showInstanced = true;
}

// ---------------------------------------------------------------------------
// Frame helpers
// ---------------------------------------------------------------------------

void
Game::updateHud() {
  float avg = 0.0f;
  for (const float delta : m_deltas) {
    avg += delta;
  }
  avg /= static_cast<float>(kDeltaSamples);

  // The HUD label is written every frame; build the transient text in the
  // frame arena instead of a heap string. setText copies the data.
  char *textBuffer = static_cast<char *>(FrameMemory::instance().allocate(160));
  if (nullptr == textBuffer) {
    return;
  }

  const size_t drawCalls =
      m_showInstanced ? InstanceDrawer::instance().getLastDrawCalls()
                      : m_stressCount;
  const float fps = avg > 0.0f ? std::round(1.0f / avg) : 0.0f;
  std::snprintf(textBuffer, 160,
                "FPS: %.0f\nNodes: %zu\nStress[B]: %s\nDraws: %zu\nVSync[V]: %s",
                fps, m_scene ? m_scene->getNodeCount() : 0u,
                m_showInstanced ? "INSTANCED" : "REGULAR", drawCalls,
                m_vsyncOn ? "on" : "off");

  if (nullptr != m_debugLabel) {
    m_debugLabel->setText(StringView(textBuffer));
  }
}

void
Game::handleInput() {
  sf::RenderWindow &window = Window::instance().getRenderWindow();

  if (Keyboard::instance().wasPressedThisFrame(Key::kEscape)) {
    window.close();
  }

  if (Keyboard::instance().wasPressedThisFrame(Key::kI) &&
      nullptr != m_particleSystem) {
    std::cout << "Current particles: " << m_particleSystem->getParticleCount()
              << std::endl;
  }

  if (Keyboard::instance().wasPressedThisFrame(Key::kB) &&
      nullptr != m_stressInstanced && nullptr != m_stressRegular) {
    m_showInstanced = !m_showInstanced;
    m_stressInstanced->setVisible(m_showInstanced);
    m_stressRegular->setVisible(!m_showInstanced);
    std::cout << "[Stress] mode: "
              << (m_showInstanced ? "INSTANCED" : "REGULAR")
              << " | sprites/grid: " << m_stressCount << std::endl;
  }

  if (Keyboard::instance().wasPressedThisFrame(Key::kV)) {
    m_vsyncOn = !m_vsyncOn;
    window.setVerticalSyncEnabled(m_vsyncOn);
    std::cout << "[Stress] vsync: " << (m_vsyncOn ? "on" : "off") << std::endl;
  }

#if USING(SFMX_DEBUG_MODE)
  // Dev hot-reload: F5 re-decodes each script's LuaAsset (its raw source in raw
  // mode) and re-binds it, so an edited .lua takes effect without restarting.
  if (AssetManager::instance().getRawScriptMode() && nullptr != m_scene &&
      Keyboard::instance().wasPressedThisFrame(Key::kF5)) {
    m_scene->forEachNode([](SceneNode *n) {
      if (auto *sc = n->getComponent<ScriptComponent>()) {
        const UUID id = sc->getScriptAssetId();
        if (id != UUID::null()) {
          static_cast<void>(AssetManager::instance().reload(id)); // re-decode
          sc->setScriptAssetId(id);                              // re-bind
        }
      }
    });
    std::cout << "[Script] hot-reloaded (F5)\n";
  }
#endif
}

} // namespace sfmx
