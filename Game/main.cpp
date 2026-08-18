#include <SFML/Graphics.hpp>

#include "config/IniFile.h"

#include "core/platform/PlatformTypes.h"
#include "input/Mapping.h"
#include "input/ActionMap.h"
#include "input/Gamepad.h"
#include "input/InputAction.h"
#include "input/InputControl.h"
#include "input/InputSystem.h"
#include "input/Keyboard.h"
#include "input/Mouse.h"

#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "scene/SourceComponent.h"
#include "scene/ScriptComponent.h"
#include "scene/SceneSerializer.h"
#include "scene/ComponentRegistry.h"
#include "scene/CanvasComponent.h"

#include "ui/Canvas.h"
#include "ui/UIEventSystem.h"
#include "ui/UIButton.h"
#include "ui/UILabel.h"
#include "ui/UIImage.h"
#include "ui/UISlider.h"
#include "ui/UITextBox.h"
#include "ui/UICheckbox.h"
#include "ui/UICheckboxGroup.h"
#include "ui/UIVerticalBox.h"
#include "ui/UIHorizontalBox.h"
#include "ui/UIScrollView.h"

#include "assets/AssetManager.h"
#include "assets/TextureAsset.h"
#include "assets/AssetCooker.h"
#include "assets/AssetImporterRegistry.h"
#include "assets/TextureCodec.h"
#include "assets/LuaCodec.h"
#include "assets/SoundCodec.h"
#include "assets/MusicCodec.h"
#include "assets/FontCodec.h"
#include "assets/FontAsset.h"

#include "ImageWebP.h"   // format module: self-registers WebP decoder + import rule

#include "core/FileSystem.h"
#include "core/Window.h"

#include "gfx/GfxRenderer.h"

#include "utils/MemoryPoolHandler.h"
#include "utils/EventSystem.h"
#include "utils/Random.h"

#include "scripts/ScriptEngine.h"

#include "DemoScene.h"
#include "DemoCook.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace sfmx;

int main(int argc, char** argv)
{
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
      // TODO: this probably needs to be loaded in runtime or something, for now we are dependent and calling this here. 
      // We might want to use LoadPlugin later in the game(?)
      imagewebp::registerModule();  // adds the .webp import rule (decoder skipped: no AssetManager in cook)
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

  const uint32 windowWidth = config.getUInt("Window", "Width", 800u);
  const uint32 windowHeight = config.getUInt("Window", "Height", 600u);
  const String windowTitle = config.getString("Window", "Title", "SFMX Game");
  const bool enableVSync = config.getBool("Window", "VSync", true);

  // The Window module owns the sf::RenderWindow and creates it on start-up.
  WindowCreateInfo windowInfo;
  windowInfo.title  = windowTitle;
  windowInfo.width  = windowWidth;
  windowInfo.height = windowHeight;
  Window::startUp(windowInfo);

  sf::RenderWindow& window = Window::instance().getRenderWindow();
  window.setVerticalSyncEnabled(enableVSync);

  // Right after the window, so the shared shader program it owns is created and
  // destroyed strictly inside the lifetime of the window's GL context.
  GfxRenderer::startUp();

  // Engine modules. Order matters: SceneManager clears its scenes at shutDown
  // (returning pooled nodes/components), so it is torn down before the pools,
  // and the AssetManager whose sf::Textures they reference is torn down last.
  MemoryPoolHandler::startUp(4096);
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
  
  ScriptEngine::startUp();
  
  // Load the cooked demo scene into a SceneManager-owned scene; fall back to
  // building it in code (dev convenience if `--cook-scene` has not run yet).
  SceneManager& scenes = SceneManager::instance();
  Scene* scenePtr = scenes.loadScene("Main", demo::kSceneFile);
  if (nullptr == scenePtr) {
    std::cerr << "[Scene] could not load " << demo::kSceneFile
              << " (run `Game --cook-scene`); building in code\n";
    scenePtr = scenes.createScene("Main");
    demo::buildDemoScene(*scenePtr, static_cast<float>(windowWidth),
                         static_cast<float>(windowHeight));
  }
  Scene& scene = *scenePtr;

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
  CompositeBinding& navComposite = uiNavigate->addComposite(CompositeType::kVector2D);
  navComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kUp), -1, false}, CompositeRole::kNegativeY, {}});
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

  InputAction* uiCancel = uiActions->addAction("Cancel", ActionValueType::kButton);
  uiCancel->addBinding(InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kEscape), -1, false});
  uiCancel->setInteraction(Interaction{InteractionType::kPress, 0.f});

  //InputSystem::instance().setActiveMapping(controls);

  UIEventSystem::startUp();

  /****************************************************************************/
  /*                                 UI Setup                                 */
  /*                                                                          */

  // Create canvas
  SceneNode* canvasNode = scene.createNode("HUDCanvas");
  auto* canvaComp = canvasNode->addComponent<CanvasComponent>();
  Canvas& uiCanvas = canvaComp->getCanvas();

  // Wire up UI navigation actions
  UIEventSystem::instance().setNavigateAction(uiNavigate);
  UIEventSystem::instance().setSubmitAction(uiSubmit);
  UIEventSystem::instance().setCancelAction(uiCancel);

  UILabel* debugLabel;
  {
    // Load fonts
    SPtr<FontAsset> fontAsset;
    constexpr const char* fontPaths[] =
    {
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
      auto* debugNode = canvasNode->createChild("DebugLabel");
      debugLabel = debugNode->addComponent<UILabel>(sf::Vector2f{float(windowWidth), 50.f});
      debugLabel->setPosition({25.0f, windowHeight - 50.0f});
      debugLabel->setFontAsset(fontAsset);
      debugLabel->setText("");
      debugLabel->setCharacterSize(22);
      debugLabel->setTextColor(sf::Color::White);
      uiCanvas.addWidget(debugLabel);
    }

    // Show upgrades menu button
    auto* upgradesNode = canvasNode->createChild("UpgradesButton");
    UIButton* upgradesBtn = upgradesNode->addComponent<UIButton>(sf::Vector2f{200.f, 50.f});
    upgradesBtn->setPosition({25.0f, 25.0f});
    upgradesBtn->syncColliderToRect();
    uiCanvas.addWidget(upgradesBtn);

    // Info label
    if (fontLoaded) {
      auto* infoNode = canvasNode->createChild("InfoLabel");
      auto* infoLabel = infoNode->addComponent<UILabel>(sf::Vector2f{400.f, 50.f});
      infoLabel->setPosition({250.0f, 25.0f});
      infoLabel->setFontAsset(fontAsset);
      infoLabel->setText("");
      infoLabel->setCharacterSize(22);
      infoLabel->setTextColor(sf::Color::White);
      uiCanvas.addWidget(infoLabel);
    }

    // Upgrades menu
    if (fontLoaded) {
      // Upgrades scroll view
      auto* upgradesMenuNode = canvasNode->createChild("UpgradesMenu");
      UIScrollView* scrollView = upgradesMenuNode->addComponent<UIScrollView>(
        sf::Vector2f{310.0f, 250.f});
      scrollView->setPosition({25.0f, 100.0f});
      scrollView->syncColliderToRect();
      scrollView->setBackgroundColor(sf::Color(255, 101, 224, 128));
      uiCanvas.addWidget(scrollView);

      // Upgrades list container
      auto* upgradesListNode = canvasNode->createChild("UpgradesList");
      UIVerticalBox* list = upgradesListNode->addComponent<UIVerticalBox>(
        sf::Vector2f{310.f, 60.f});
      list->setPadding({15.0f, 10.0f});
      list->setSpacing(5.0f);
      list->setBoxColor(sf::Color::Transparent);
      scrollView->addChild(list);
      
      // Helper local function to add upgrade entries
      auto addBuyUnitButton = [&](const char* name) {
        // Upgrade container
        auto* hboxNode = canvasNode->createChild(String(name) + " HBox");
        UIHorizontalBox* hbox = hboxNode->addComponent<UIHorizontalBox>(
          sf::Vector2f{280.f, 50.f});
        hbox->setPosition({0.0f, 0.0f});
        hbox->setPadding({10.0f, 10.0f});
        hbox->setSpacing(10.f);
        hbox->setBoxColor(sf::Color(40, 40, 55, 200));
        list->addChild(hbox);
        
        // Upgrade name label
        auto* ln = canvasNode->createChild(String(name) + " Label");
        auto* lbl = ln->addComponent<UILabel>(sf::Vector2f{200.f, 30.f});
        lbl->setPosition({0.f, 0.f});
        lbl->setFontAsset(fontAsset);
        lbl->setText(name);
        lbl->setCharacterSize(13);
        lbl->setTextColor(sf::Color::White);
        hbox->addChild(lbl);

        // Upgrade button
        auto* n = canvasNode->createChild(String(name) + " Button");
        auto* btn = n->addComponent<UIButton>(sf::Vector2f{50.f, 30.f});
        btn->setPosition({0.f, 0.f});
        hbox->addChild(btn);
        
        hbox->updateLayout();
      };
      
      // Buy quantity slider
      {
        // Buy label
        auto* buyLabelNode = canvasNode->createChild("BuyLabel");
        auto* label = buyLabelNode->addComponent<UILabel>(sf::Vector2f{180.f, 22.f});
        label->setPosition({0.f, 0.f});
        label->setFontAsset(fontAsset);
        label->setText("Amount of units to buy");
        label->setCharacterSize(14);
        label->setTextColor(sf::Color::White);
        list->addChild(label);

        // Buy slider
        auto* buySliderNode = canvasNode->createChild("BuySlider");
        UISlider* buySlider = buySliderNode->addComponent<UISlider>(sf::Vector2f{180.f, 20.f});
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

    // Exit game button
    auto* btnExitNode = canvasNode->createChild("ExitBtn");
    UIButton* btnExit = btnExitNode->addComponent<UIButton>(sf::Vector2f{200.f, 50.f});
    btnExit->setPosition({windowWidth - 225.0f,
                          windowHeight - 75.0f});
    btnExit->syncColliderToRect();
    btnExit->setNormalColor(sf::Color(180, 80, 80));
    uiCanvas.addWidget(btnExit);
  }

  /*                                                                          */
  /*                                 UI Setup                                 */
  /****************************************************************************/
  
  SceneNode* gameManager = scene.createNode("GameManager");
  gameManager->addComponent<ScriptComponent>(sfmx::UUID::createFromName("gameManager.lua"));

  sf::Clock clock;

  constexpr size_t deltasSize = 100;
  std::array<float, deltasSize> deltas;
  uint32 index = 0;

  while (window.isOpen())
  {
    // InputSystem: snapshot device state before polling
    InputSystem::instance().beginFrame();

    while (const Optional<sf::Event> event = window.pollEvent())
    {
      if (event->is<sf::Event::Closed>())
      {
        window.close();
      }
      else if (const auto* text = event->getIf<sf::Event::TextEntered>())
      {
        if (auto* textBox = dynamic_cast<UITextBox*>(
              UIEventSystem::instance().getSelected())) {
          const char32_t ch = text->unicode;
          if (ch == 8) {
            textBox->deleteCharacter();
          } else if (ch >= 32) {
            textBox->insertCharacter(static_cast<uint32>(ch));
          }
        }
      }

      InputSystem::instance().onEvent(*event);
    }

    const float deltaTime = clock.restart().asSeconds();

    deltas[index] = deltaTime;
    index = (index + 1) % deltasSize;
    float avg = 0.0f;
    for (uint32 i = 0; i < deltasSize; ++i) avg += deltas[index];
    avg /= static_cast<float>(deltasSize);

    debugLabel->setText(std::format("FPS: {0}\nNodes: {1}", std::round(1.0f / avg), scene.getNodeCount()));
    
    
    InputSystem::instance().update(deltaTime, window);
    
    if (Keyboard::instance().wasPressedThisFrame(Key::kEscape)) {
      window.close();
    }

#if USING(SFMX_DEBUG_MODE)
    // Dev hot-reload: F5 re-decodes each script's LuaAsset (its raw source in raw mode)
    // and re-binds it, so an edited .lua takes effect without restarting the game.
    if (AssetManager::instance().getRawScriptMode() &&
        Keyboard::instance().wasPressedThisFrame(Key::kF5)) {
      scene.forEachNode([](SceneNode* n) {
        if (auto* sc = n->getComponent<ScriptComponent>()) {
          const sfmx::UUID id = sc->getScriptAssetId();
          if (id != sfmx::UUID::null()) {
            static_cast<void>(AssetManager::instance().reload(id));  // re-decode (raw re-read)
            sc->setScriptAssetId(id);                                // re-bind (recompile)
          }
        }
      });
      std::cout << "[Script] hot-reloaded (F5)\n";
    }
#endif

    // Finalize any assets whose background decode completed (GPU upload on this,
    // the GL-owning thread) and fire their loadAsync callbacks BEFORE the scene
    // updates, so components/scripts see freshly loaded assets this same frame.
    AssetManager::instance().finalize();

    UIEventSystem::instance().update(window, deltaTime);
    SceneManager::instance().update(deltaTime);

    window.clear(sf::Color(24, 24, 28));

    scenes.draw(window);

    // Screen-space canvas: reset the view so coordinates match window pixels.
    window.setView(window.getDefaultView());
    uiCanvas.draw(window, sf::RenderStates::Default);

    window.display();
  }

  // Release any pending async-load callbacks (they may hold Lua closures) and tear down
  // the scenes (their ScriptComponents hold Lua handles) while the script engine / Lua
  // state is still alive — ScriptEngine / AssetManager shut down just below.
  AssetManager::instance().cancelAsyncLoads();
  SceneManager::instance().destroyAllScenes();

  UIEventSystem::shutDown();

  ScriptEngine::shutDown();
  AssetManager::shutDown();
  // Shut the scene manager down before the pools: it clears every scene, which
  // returns pooled nodes/components while the pools (and SFML) are still alive.
  SceneManager::shutDown();
  ComponentRegistry::shutDown();

  PhysicsSystem::shutDown();
  InputSystem::shutDown();
  MemoryPoolHandler::shutDown();

  // Before the window, for the same reason its start-up came after: the shader
  // program it owns has to be released while the GL context still exists.
  GfxRenderer::shutDown();

  // Shut the window down last: keep its GL context alive until every sf::Texture
  // owned by the (now torn-down) AssetManager has been released.
  Window::shutDown();

  return 0;
}
