#include <SFML/Graphics.hpp>

#include "config/IniFile.h"

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

#include "utils/MemoryPoolHandler.h"
#include "utils/EventSystem.h"
#include "utils/Random.h"

#include "scripts/ScriptEngine.h"

#include "DemoScene.h"
#include "DemoCook.h"

#include <array>
#include <cmath>
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
  ActionMap* gameplay = controls->addMap("Gameplay");

  InputAction* jump = gameplay->addAction("Jump", ActionValueType::kButton);
  jump->addBinding(InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kSpace), -1, false});
  Interaction tap;
  tap.m_type = InteractionType::kTap;
  tap.m_duration = 0.2f;
  jump->setInteraction(tap);

  InputAction* crouch = gameplay->addAction("Crouch", ActionValueType::kButton);
  crouch->addBinding(InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kLControl), -1, false});
  Interaction hold;
  hold.m_type = InteractionType::kHold;
  hold.m_duration = 0.4f;
  crouch->setInteraction(hold);

  InputAction* move = gameplay->addAction("Move", ActionValueType::kAxis2D);
  CompositeBinding& moveComposite = move->addComposite(CompositeType::kVector2D);
  moveComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kA), -1, false}, CompositeRole::kNegativeX, {}});
  moveComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kD), -1, false}, CompositeRole::kPositiveX, {}});
  moveComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kS), -1, false}, CompositeRole::kNegativeY, {}});
  moveComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, static_cast<int32>(Key::kW), -1, false}, CompositeRole::kPositiveY, {}});
  move->addProcessor(Processor{ProcessorType::kNormalize, {}, {}});

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

  InputSystem::instance().setActiveMapping(controls);

  HEvent jumpSub = jump->onPerformed([](const InputContext&) {
    std::cout << "[Action] Jump performed\n";
  });
  HEvent crouchSub = crouch->onPerformed([](const InputContext&) {
    std::cout << "[Action] Crouch performed (held past threshold)\n";
  });
  HEvent crouchStart = crouch->onStarted([](const InputContext&) {
    std::cout << "[Action] Crouch started\n";
  });
  HEvent crouchEnd = crouch->onCanceled([](const InputContext&) {
    std::cout << "[Action] Crouch canceled\n";
  });
  float moveReportTimer = 0.1f;
  HEvent moveSub = move->onPerformed([&moveReportTimer](const InputContext& ctx) {
    // Performed fires every non-zero frame; throttle the print.
    moveReportTimer += ctx.m_deltaTime;
    if (moveReportTimer >= 0.1f) {
      const Vector2f value = ctx.m_value.asVector2();
      std::cout << "[Action] Move (" << value.x << ", " << value.y << ")\n";
      moveReportTimer = 0.f;
    }
  });

  UIEventSystem::startUp();
  
  SceneNode* canvasNode = scene.createNode("HUDCanvas");
  auto* canvaComp = canvasNode->addComponent<CanvasComponent>();
  Canvas& uiCanvas = canvaComp->getCanvas();

  auto* btnNode = canvasNode->createChild("StartBtn");
  
  UIButton* btn = btnNode->addComponent<UIButton>(sf::Vector2f{200.f, 50.f});
  btn->setPosition({windowWidth * 0.5f - 100.f, windowHeight * 0.5f - 25.f});
  btn->syncColliderToRect();
  uiCanvas.addWidget(btn);

  auto* btnExitNode = canvasNode->createChild("ExitBtn");
  UIButton* btnExit = btnExitNode->addComponent<UIButton>(sf::Vector2f{200.f, 50.f});
  btnExit->setPosition({windowWidth * 0.5f - 100.f,
                        windowHeight * 0.5f + 40.f});
  btnExit->syncColliderToRect();
  btnExit->setNormalColor(sf::Color(180, 80, 80));
  uiCanvas.addWidget(btnExit);

  // Wire up UI navigation actions
  UIEventSystem::instance().setNavigateAction(uiNavigate);
  UIEventSystem::instance().setSubmitAction(uiSubmit);
  UIEventSystem::instance().setCancelAction(uiCancel);

  // Explicit navigation links
  btn->setNavDown(btnExit);
  btnExit->setNavUp(btn);

  int clickCount = 0;
  HEvent btnSub = btn->onPointerClick([&clickCount](sf::Vector2f pos) {
    ++clickCount;
    std::cout << "[UI] Start clicked (" << clickCount << "x) at ("
              << pos.x << ", " << pos.y << ")\n";
  });
  HEvent btnSubNav = btn->onSubmit([&clickCount]() {
    ++clickCount;
    std::cout << "[UI] Start submitted via keyboard (" << clickCount << "x)\n";
  });
  HEvent exitSub = btnExit->onPointerClick([&window](sf::Vector2f pos) {
    SFMX_PARAMETER_UNUSED(pos);
    std::cout << "[UI] Exit clicked - closing window\n";
    window.close();
  });
  HEvent exitSubNav = btnExit->onSubmit([&window]() {
    std::cout << "[UI] Exit submitted via keyboard - closing window\n";
    window.close();
  });

  // ── UILabel demo ──────────────────────────────────────────────────────
  // auto font = MakeShared<sf::Font>();
  // Try common font paths across Linux distros.
  SPtr<FontAsset> fontAsset;
#if SFMX_PLATFORM_WINDOWS
  constexpr const char* fontPaths[] =
  {
    "ARIAL.TTF",
    "C:\\Windows\\Fonts\\segoeui.ttf",
    "C:\\Windows\\Fonts\\arial.ttf",
    "C:\\Windows\\Fonts\\tahoma.ttf",
    "C:\\Windows\\Fonts\\calibri.ttf"
  };
#elif SFMX_PLATFORM_MACOS
  constexpr const char* fontPaths[] =
  {
    "ARIAL.TTF",
    "/System/Library/Fonts/SFNS.ttf",
    "/System/Library/Fonts/Supplemental/Arial.ttf",
    "/Library/Fonts/Arial.ttf"
  };
#else // Linux
  constexpr const char* fontPaths[] =
  {
    "ARIAL.TTF",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
    "/usr/share/fonts/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/noto/NotoSans-Regular.ttf",
    "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf"
  };
#endif
  bool fontLoaded = false;
  
  for (const char* fp : fontPaths) {
    fontAsset = AssetManager::instance().load<FontAsset>(
      sfmx::UUID::createFromName(String(fp)));
    if (fontAsset && fontAsset->isLoaded()) {
      fontLoaded = true;
      break;
    }
  }

  if (fontLoaded) {
    auto* lblNode = canvasNode->createChild("TitleLabel");
    auto* label = lblNode->addComponent<UILabel>(sf::Vector2f{400.f, 40.f});
    label->setPosition({windowWidth * 0.5f - 200.f, 20.f});
    label->setFontAsset(fontAsset);
    label->setText("SFMX Engine - UI Widget Demo");
    label->setCharacterSize(22);
    label->setTextColor(sf::Color::White);
    uiCanvas.addWidget(label);
  } else {
    std::cout << "[UI] Could not load DejaVuSans font; skipping label\n";
  }

  // ── UIImage demo ───────────────────────────────────────────────────────
  SPtr<TextureAsset> uiTex = AssetManager::instance().load<TextureAsset>(
      sfmx::UUID::createFromName("particle.png"));
  if (uiTex) {
    auto* imgNode = canvasNode->createChild("DemoImage");
    auto* image = imgNode->addComponent<UIImage>(sf::Vector2f{32.f, 32.f});
    image->setPosition({20.f, 20.f});
    image->setTextureAsset(uiTex);
    uiCanvas.addWidget(image);
  }

  // ── UISlider demo ──────────────────────────────────────────────────────
  auto* sliderNode = canvasNode->createChild("DemoSlider");
  UISlider* slider = sliderNode->addComponent<UISlider>(sf::Vector2f{250.f, 20.f});
  slider->setPosition({windowWidth * 0.5f - 125.f, windowHeight * 0.85f});
  slider->syncColliderToRect();
  slider->setRange(0.f, 100.f);
  slider->setValue(50.f);
  uiCanvas.addWidget(slider);

  HEvent slSub = slider->onValueChanged([](float val) {
    std::cout << "[UI] Slider value: " << val << "\n";
  });

  // ── UICheckbox demo ────────────────────────────────────────────────────
  auto* cbNode = canvasNode->createChild("DemoCheckbox");
  UICheckbox* checkbox = cbNode->addComponent<UICheckbox>(sf::Vector2f{28.f, 28.f});
  checkbox->setPosition({60.f, windowHeight * 0.5f});
  checkbox->syncColliderToRect();
  uiCanvas.addWidget(checkbox);

  HEvent cbSub = checkbox->onValueChanged([](bool checked) {
    std::cout << "[UI] Checkbox: " << (checked ? "checked" : "unchecked") << "\n";
  });

  // ── ScrollView + VerticalBox (layout container demo) ──────────────────
  SPtr<UICheckboxGroup> radioGroup;
  Vector<HEvent> groupLogs;

  if (fontLoaded) {
    auto* scrollNode = canvasNode->createChild("DemoScrollView");
    UIScrollView* scrollView = scrollNode->addComponent<UIScrollView>(
      sf::Vector2f{260.f, 280.f});
    scrollView->setPosition({windowWidth * 0.5f + 50.f, 80.f});
    scrollView->syncColliderToRect();
    scrollView->setBackgroundColor(sf::Color(30, 30, 40, 220));
    uiCanvas.addWidget(scrollView);

    auto* listNode = canvasNode->createChild("DemoList");
    UIVerticalBox* list = listNode->addComponent<UIVerticalBox>(
      sf::Vector2f{240.f, 60.f});  // initial size, will stretch
    list->setPadding({8.f, 8.f});
    list->setSpacing(6.f);
    list->setBoxColor(sf::Color::Transparent);
    scrollView->addChild(list);

    // Populate the list with checkboxes + labels
    auto addItem = [&](const char* name, bool checked) {
      auto* rowNode = canvasNode->createChild(String(name) + "Row");
      auto* cb = rowNode->addComponent<UICheckbox>(sf::Vector2f{22.f, 22.f});
      cb->setPosition({0.f, 0.f});  // layout managed by the box
      list->addChild(cb);
      if (checked) cb->setChecked(true, false);

      auto* lblNode = canvasNode->createChild(String(name) + "Lbl");
      auto* lbl = lblNode->addComponent<UILabel>(sf::Vector2f{180.f, 22.f});
      lbl->setPosition({28.f, 0.f});
      lbl->setFontAsset(fontAsset);
      lbl->setText(name);
      lbl->setCharacterSize(14);
      lbl->setTextColor(sf::Color::White);
      list->addChild(lbl);

      groupLogs.push_back(cb->onValueChanged([n = String(name)](bool v) {
        std::cout << "[ScrollItem] " << n << ": " << (v ? "checked" : "unchecked") << "\n";
      }));
    };

    addItem("SFML Integration", true);
    addItem("UI Widget System", true);
    addItem("Scene Graph",      true);
    addItem("Physics (Box2D)",  true);
    addItem("Particle System",  true);
    addItem("Animation System", true);
    addItem("Audio System",     true);
    addItem("Input System",     true);
    addItem("Lua Scripting",    true);
    addItem("Asset Pipeline",   false);
    addItem("Serialization",    false);
    addItem("Profiling Tools",  false);

    // Add a submit button inside the list
    auto* btnNode = canvasNode->createChild("SubmitBtn");
    UIButton* submitBtn = btnNode->addComponent<UIButton>(sf::Vector2f{100.f, 28.f});
    submitBtn->setPosition({0.f, 0.f});
    submitBtn->setNormalColor(sf::Color(70, 140, 70));
    submitBtn->setHoveredColor(sf::Color(90, 180, 90));
    submitBtn->setPressedColor(sf::Color(50, 100, 50));
    list->addChild(submitBtn);

    HEvent sbSub = submitBtn->onPointerClick([](sf::Vector2f) {
      std::cout << "[ScrollItem] Submit button clicked!\n";
    });

    // Add a UIImage inside the list (uses the particle texture if available)
    if (uiTex) {
      auto* imgNode = canvasNode->createChild("ScrollImg");
      UIImage* scrollImg = imgNode->addComponent<UIImage>(sf::Vector2f{32.f, 32.f});
      scrollImg->setPosition({0.f, 0.f});
      scrollImg->setTextureAsset(uiTex);
      scrollImg->setColor(sf::Color(255, 200, 100));
      list->addChild(scrollImg);
    }

    // Add a UITextBox inside the list
    auto* tbxNode = canvasNode->createChild("ScrollTbx");
    UITextBox* scrollTbx = tbxNode->addComponent<UITextBox>(sf::Vector2f{224.f, 28.f});
    scrollTbx->setPosition({0.f, 0.f});
    scrollTbx->setFontAsset(fontAsset);
    scrollTbx->setCharacterSize(14);
    scrollTbx->setPlaceholder("Enter text...");
    list->addChild(scrollTbx);

    // Add a volume slider inside the list
    auto* volNode = canvasNode->createChild("VolSlider");
    UISlider* volSlider = volNode->addComponent<UISlider>(sf::Vector2f{180.f, 20.f});
    volSlider->setPosition({0.f, 0.f});
    volSlider->setRange(0.f, 100.f);
    volSlider->setValue(75.f);
    list->addChild(volSlider);

    HEvent vsSub = volSlider->onValueChanged([](float v) {
      std::cout << "[ScrollItem] Volume: " << v << "\n";
    });

    list->updateLayout();

    // Fit the box to content height, scroll view handles overflow
    float contentH = 8.f; // top padding
    for (auto* child : list->getChildren()) {
      contentH += child->getSize().y + 6.f;
    }
    list->setSize({list->getSize().x, contentH});
    scrollView->setContentHeight(contentH);

    // --- Exclusive radio group below the scroll view ---
    radioGroup = MakeShared<UICheckboxGroup>();
    radioGroup->setExclusive(true);

    auto* radioTitle = canvasNode->createChild("RadioGroupTitle");
    auto* radioTitleLbl = radioTitle->addComponent<UILabel>(sf::Vector2f{200.f, 24.f});
    radioTitleLbl->setPosition({windowWidth * 0.5f + 50.f, 380.f});
    radioTitleLbl->setFontAsset(fontAsset);
    radioTitleLbl->setText("Render backend:");
    radioTitleLbl->setCharacterSize(14);
    radioTitleLbl->setTextColor(sf::Color::White);
    uiCanvas.addWidget(radioTitleLbl);

    static constexpr const char* kOpts[] = {"OpenGL", "Vulkan", "DirectX"};
    float rx = windowWidth * 0.5f + 50.f;
    const float ry = 405.f;
    for (auto* opt : kOpts) {
      auto* node = canvasNode->createChild(String(opt) + "Rb");
      auto* cb = node->addComponent<UICheckbox>(sf::Vector2f{22.f, 22.f});
      cb->setPosition({rx, ry});
      cb->syncColliderToRect();
      cb->setGroup(radioGroup.get());
      uiCanvas.addWidget(cb);

      auto* lblNode = canvasNode->createChild(String(opt) + "RbLbl");
      auto* lbl = lblNode->addComponent<UILabel>(sf::Vector2f{80.f, 22.f});
      lbl->setPosition({rx + 28.f, ry + 1.f});
      lbl->setFontAsset(fontAsset);
      lbl->setText(opt);
      lbl->setCharacterSize(14);
      lbl->setTextColor(sf::Color::White);
      uiCanvas.addWidget(lbl);

      groupLogs.push_back(cb->onValueChanged([n = String(opt)](bool v) {
        std::cout << "[Radio] " << n << (v ? " selected" : " deselected") << "\n";
      }));

      rx += 85.f;
    }
  }

  // ── HorizontalBox demo ──────────────────────────────────────────────────
  if (fontLoaded) {
    auto* hboxNode = canvasNode->createChild("DemoHBox");
    UIHorizontalBox* hbox = hboxNode->addComponent<UIHorizontalBox>(
      sf::Vector2f{420.f, 40.f});
    hbox->setPosition({20.f, windowHeight * 0.5f + 60.f});
    hbox->setPadding({6.f, 8.f});
    hbox->setSpacing(8.f);
    hbox->setBoxColor(sf::Color(40, 40, 55, 200));
    uiCanvas.addWidget(hbox);

    // Add a few label + value pairs as children
    auto addHItem = [&](const char* text) {
      auto* n = canvasNode->createChild(String("HBox_") + text);
      auto* btn = n->addComponent<UIButton>(sf::Vector2f{60.f, 24.f});
      btn->setPosition({0.f, 0.f});
      hbox->addChild(btn);

      auto* ln = canvasNode->createChild(String("HBoxLbl_") + text);
      auto* lbl = ln->addComponent<UILabel>(sf::Vector2f{60.f, 24.f});
      lbl->setPosition({64.f, 0.f});
      lbl->setFontAsset(fontAsset);
      lbl->setText(text);
      lbl->setCharacterSize(13);
      lbl->setTextColor(sf::Color::White);
      hbox->addChild(lbl);
    };
    addHItem("Cut");
    addHItem("Copy");
    addHItem("Paste");
    addHItem("Undo");

    hbox->updateLayout();
  }

  // ── Nested VerticalBox demo (inside the main canvas, left side) ─────────
  if (fontLoaded) {
    auto* vboxNode = canvasNode->createChild("DemoVBox");
    UIVerticalBox* vbox = vboxNode->addComponent<UIVerticalBox>(
      sf::Vector2f{160.f, 180.f});
    vbox->setPosition({20.f, 80.f});
    vbox->setPadding({8.f, 8.f});
    vbox->setSpacing(4.f);
    vbox->setBoxColor(sf::Color(40, 40, 55, 200));
    uiCanvas.addWidget(vbox);

    auto* titleN = canvasNode->createChild("VBoxTitle");
    auto* titleLbl = titleN->addComponent<UILabel>(sf::Vector2f{144.f, 20.f});
    titleLbl->setPosition({0.f, 0.f});
    titleLbl->setFontAsset(fontAsset);
    titleLbl->setText("Quick Actions");
    titleLbl->setCharacterSize(14);
    titleLbl->setTextColor(sf::Color(200, 200, 255));
    vbox->addChild(titleLbl);

    auto addVItem = [&](const char* text, const sf::Color& color) {
      auto* n = canvasNode->createChild(String("VBoxBtn_") + text);
      auto* btn = n->addComponent<UIButton>(sf::Vector2f{144.f, 24.f});
      btn->setPosition({0.f, 0.f});
      btn->setNormalColor(color);
      btn->setHoveredColor(sf::Color(
        static_cast<uint8>(std::min(255, color.r + 30)),
        static_cast<uint8>(std::min(255, color.g + 30)),
        static_cast<uint8>(std::min(255, color.b + 30))));
      btn->setPressedColor(sf::Color(
        static_cast<uint8>(std::max(0, color.r - 30)),
        static_cast<uint8>(std::max(0, color.g - 30)),
        static_cast<uint8>(std::max(0, color.b - 30))));
      vbox->addChild(btn);

      groupLogs.push_back(btn->onPointerClick([n = String(text)](sf::Vector2f) {
        std::cout << "[VBox] " << n << " clicked\n";
      }));
    };
    addVItem("New File",    sf::Color(60, 120, 60));
    addVItem("Open...",     sf::Color(60, 60, 140));
    addVItem("Save",        sf::Color(60, 100, 160));
    addVItem("Save As...",  sf::Color(100, 60, 60));
    addVItem("Settings",    sf::Color(80, 80, 80));

    vbox->updateLayout();
  }

  // ── UITextBox demo ─────────────────────────────────────────────────────
  if (fontLoaded) {
    auto* textBoxNode = canvasNode->createChild("DemoTextBox");
    UITextBox* textBox = textBoxNode->addComponent<UITextBox>(sf::Vector2f{300.f, 40.f});
    textBox->setPosition({windowWidth * 0.5f - 150.f, windowHeight * 0.75f});
    textBox->syncColliderToRect();
    textBox->setFontAsset(fontAsset);
    textBox->setCharacterSize(20);
    textBox->setPlaceholder("Type here...");
    uiCanvas.addWidget(textBox);
  }

  std::cout << "[UI] System ready - interact with the widgets\n"
            << "[UI] Navigate: Arrow keys / WASD  |  Submit: Space / Enter  |  Cancel: Escape\n";

  sf::Clock clock;

  std::cout << "Random demo:\n"
            << "Get: "    << Random::get<float>()       << "\n"
            << "Get: "    << Random::get<float>()       << "\n"
            << "Get: "    << Random::get<float>()       << "\n"
            << "Range: "  << Random::range<int>(0, 30)  << "\n"
            << "Range: "  << Random::range<int>(0, 30)  << "\n"
            << "Range: "  << Random::range<int>(0, 30)  << "\n"
            << "Dice: "   << Random::diceThrow(3, 6)    << "\n"
            << "Dice: "   << Random::diceThrow(2, 6)    << "\n"
            << "Dice: "   << Random::diceThrow(1, 6)    << "\n";

  std::array<float, 10> avgFPS = {};
  for (float& fps : avgFPS) fps = 0.0069f;
  uint32 avgFPSIndex = 0;

  bool render = true;
  bool showInfo = false;

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

    // InputSystem: here is where the mappings are being executed
    InputSystem::instance().update(deltaTime, window);

    // InputSystem: example of "Direct Mode".
    if (Keyboard::instance().wasPressedThisFrame(Key::kEscape)) {
      window.close();
    }

    avgFPS[avgFPSIndex] = deltaTime;
    avgFPSIndex = (avgFPSIndex + 1) % avgFPS.size();

    if (Keyboard::instance().wasPressedThisFrame(Key::kNum1)) {
      showInfo = !showInfo;
    }
    if (Keyboard::instance().wasPressedThisFrame(Key::kNum2)) {
      render = !render;
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
    
    if (Keyboard::instance().wasPressedThisFrame(Key::kI) || (showInfo && avgFPSIndex == 0u)) {
      float avgDeltaTime = 0.f;
      for (const float& dt : avgFPS) {
        avgDeltaTime += dt;
      }
      avgDeltaTime /= avgFPS.size();

      std::cout << "[Info] FPS: " << std::ceil(1.0f / avgDeltaTime) << "\n";
      std::cout << "[Info] Scene total nodes: " << SceneManager::instance().getActiveScene()->getNodeCount() << "\n";
      demo::poolsInfo();
    }

    // Rotating the parent drags the child with it: proof of transform
    // composition down the hierarchy.
    if (nullptr != rt.sun)     { rt.sun->transform().rotate(sf::degrees(45.f * deltaTime)); }
    if (nullptr != rt.sun2)    { rt.sun2->transform().rotate(sf::degrees(10.f * deltaTime)); }
    if (nullptr != rt.earth)   { rt.earth->transform().rotate(sf::degrees(215.f * deltaTime)); }
    if (nullptr != rt.neptune) { rt.neptune->transform().rotate(sf::degrees(-15.f * deltaTime)); }

    UIEventSystem::instance().update(window, deltaTime);
    SceneManager::instance().update(deltaTime);

    window.clear(sf::Color(24, 24, 28));
    if (render) {
      scenes.draw(window);

      // Screen-space canvas: reset the view so coordinates match window pixels.
      window.setView(window.getDefaultView());
      uiCanvas.draw(window, sf::RenderStates::Default);
    }
    window.display();
  }

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

  // Shut the window down last: keep its GL context alive until every sf::Texture
  // owned by the (now torn-down) AssetManager has been released.
  Window::shutDown();

  return 0;
}
