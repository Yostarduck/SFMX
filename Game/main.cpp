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
#include "scene/SceneSerializer.h"
#include "scene/ComponentRegistry.h"
#include "scene/CanvasComponent.h"

#include "ui/Canvas.h"
#include "ui/UIEventSystem.h"
#include "ui/UIButton.h"

#include "utils/MemoryPoolHandler.h"
#include "utils/EventSystem.h"

#include "scripts/ScriptEngine.h"

#include "assets/AssetCooker.h"
#include "assets/AssetManager.h"
#include "assets/TextureCodec.h"

#include "DemoScene.h"
#include "DemoCook.h"

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
      AssetCooker::cookDirectory(srcDir, outDir);
      return 0;
    }
    if (std::strcmp(argv[i], "--cook-scene") == 0) {
      return demo::cookScene();
    }
  }

  IniFile config;
  config.loadAll({"Game/config/Engine.ini", "Game/config/Game.ini"});

  const uint32 windowWidth = config.getUInt("Window", "Width", 800u);
  const uint32 windowHeight = config.getUInt("Window", "Height", 600u);
  const String windowTitle = config.getString("Window", "Title", "SFMX Game");
  const bool enableVSync = config.getBool("Window", "VSync", true);

  sf::RenderWindow window(sf::VideoMode({windowWidth, windowHeight}), windowTitle);
  window.setVerticalSyncEnabled(enableVSync);

  // Engine modules. Order matters: SceneManager clears its scenes at shutDown
  // (returning pooled nodes/components), so it is torn down before the pools,
  // and the AssetManager whose sf::Textures they reference is torn down last.
  InputSystem::startUp();
  PhysicsSystem::startUp();
  MemoryPoolHandler::startUp(4096);
  ScriptEngine::startUp();
  ComponentRegistry::startUp();
  SceneManager::startUp();

  demo::registerDemoPools(MemoryPoolHandler::instance());
  demo::registerDemoComponents();

  // Mount the cooked .sfmxasset directory (the build's POST_BUILD step runs
  // `Game --cook` then `Game --cook-scene`, so Game/assets is populated). Images
  // resolve by UUID through the AssetManager; audio stays mp3-by-path (streams).
  AssetManager::startUp();
  AssetManager::instance().registerCodec(MakeShared<TextureCodec>());
  const size_t mountedAssets = AssetManager::instance().mount("Game/assets");
  std::cout << "[Assets] mounted " << mountedAssets << " from Game/assets\n";

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

  // InputSystem: "Mapping Mode" demo — a Mapping holds an ActionMap, which holds
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

  sf::Clock clock;

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
    std::cout << "[UI] Exit clicked — closing window\n";
    window.close();
  });
  HEvent exitSubNav = btnExit->onSubmit([&window]() {
    std::cout << "[UI] Exit submitted via keyboard — closing window\n";
    window.close();
  });

  std::cout << "[UI] System ready — click buttons or press Escape\n"
            << "[UI] Navigate: Arrow keys / WASD  |  Submit: Space / Enter  |  Cancel: Escape\n";

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

      InputSystem::instance().onEvent(*event);
    }

    const float deltaTime = clock.restart().asSeconds();

    // InputSystem: here is where the mappings are being executed
    InputSystem::instance().update(deltaTime, window);

    // InputSystem: example of "Direct Mode".
    if (Keyboard::instance().wasPressedThisFrame(Key::kEscape)) {
      window.close();
    }
    if (Keyboard::instance().wasPressedThisFrame(Key::kSpace)) {
      std::cout << "[Input] Space pressed\n";
      if (nullptr != rt.moonSfx) {
        rt.moonSfx->stop();
        rt.moonSfx->play();
      }
    }
    if (Mouse::instance().wasPressedThisFrame(MouseButton::kLeft)) {
      const Vector2i pos = Mouse::instance().getPosition();
      std::cout << "[Input] Left click at (" << pos.x << ", " << pos.y << ")\n";
    }
    if (Gamepad::instance().isConnected(0)) {
      const float lx = Gamepad::instance().get(0).getAxis(Axis::kLeftX);
      if (lx != 0.f) {
        std::cout << "[Input] Gamepad 0 LeftX = " << lx << "\n";
      }
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
    scenes.draw(window);
    // Screen-space canvas: reset the view so coordinates match window pixels.
    window.setView(window.getDefaultView());
    uiCanvas.draw(window, sf::RenderStates::Default);
    window.display();
  }

  ScriptEngine::shutDown();

  // Shut the scene manager down before the pools: it clears every scene, which
  // returns pooled nodes/components while the pools (and SFML) are still alive.
  SceneManager::shutDown();
  ComponentRegistry::shutDown();

  InputSystem::shutDown();
  UIEventSystem::shutDown();
  PhysicsSystem::shutDown();
  MemoryPoolHandler::shutDown();
  AssetManager::shutDown();

  return 0;
}
