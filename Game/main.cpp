#include <SFML/Graphics.hpp>

#include "config/IniFile.h"
#include "input/ActionMap.h"
#include "input/Gamepad.h"
#include "input/InputAction.h"
#include "input/InputControl.h"
#include "input/InputSystem.h"
#include "input/Keyboard.h"
#include "input/Mapping.h"
#include "input/Mouse.h"
#include "scene/ListenerComponent.h"
#include "scene/Scene.h"
#include "scene/SourceComponent.h"
#include "utils/Module.h"
#include "utils/EventSystem.h"
#include "utils/Random.h"
#include "utils/Arithmetic.h"

using namespace sfmx;

class CircleComponent : public ComponentT<CircleComponent>
{
 public:
  CircleComponent(SceneNode* owner, float radius, sf::Color color)
    : ComponentT<CircleComponent>(owner)
  {
    m_circle.setRadius(radius);
    m_circle.setFillColor(color);
    m_circle.setOrigin({radius, radius});
  }

  void
  onDraw(sf::RenderTarget& target, sf::RenderStates states) const override {
    target.draw(m_circle, states);
  }

 private:
  sf::CircleShape m_circle;
};

DECLARE_TYPE_TRAITS(CircleComponent)
DECLARE_TYPE_TRAITS(SourceComponent)
DECLARE_TYPE_TRAITS(ListenerComponent)

int main()
{
  sfmx::IniFile config;
  config.loadAll({"Game/config/Engine.ini", "Game/config/Game.ini"});

  const uint32 windowWidth = config.getUInt("Window", "Width", 800u);
  const uint32 windowHeight = config.getUInt("Window", "Height", 600u);
  const String windowTitle = config.getString("Window", "Title", "SFMX Game");
  const bool enableVSync = config.getBool("Window", "VSync", true);

  sf::RenderWindow window(sf::VideoMode({windowWidth, windowHeight}), windowTitle);
  window.setVerticalSyncEnabled(enableVSync);

  InputSystem::startUp();
  
  Scene scene("Main", 1024);
  scene.registerComponent<CircleComponent>(64);
  scene.registerComponent<SourceComponent>(4);
  scene.registerComponent<ListenerComponent>(1);

  SceneNode* sun = scene.createNode("Sun");
  sun->transform().setPosition(
      {static_cast<float>(windowWidth) * 0.5f,
       static_cast<float>(windowHeight) * 0.5f});
  sun->addComponent<CircleComponent>(40.f, sf::Color(255, 180, 100));
  sf::Vector2f center = {static_cast<float>(windowWidth) * 0.5f,
                         static_cast<float>(windowHeight) * 0.5f};
  SceneNode* sun2 = scene.createNode("Sun2");
  sun2->transform().setPosition(center);
  

  SceneNode* earth = scene.createNode("Earth", sun);
  earth->transform().setPosition({140.f, 0.f});
  earth->addComponent<CircleComponent>(20.f, sf::Color(100, 180, 255));
  auto* bgm = earth->addComponent<SourceComponent>();
  {
    bgm->setFollowNode(true);
    if (bgm->loadMusicFromFile("Game/resources/background.mp3")) {
      bgm->setLooping(true);
      bgm->setVolume(10.f);
      bgm->setSpatializationEnabled(false);
      bgm->play();
    } else {
      std::cout << "[Audio] Failed to load background.mp3\n";
    }
  }

  SceneNode* moon = scene.createNode("Moon", earth);
  moon->transform().setPosition({40.f, 0.f});
  moon->addComponent<CircleComponent>(4.f, sf::Color(100, 100, 100));
  SourceComponent* moonSfx = nullptr;
  auto* sfx = moon->addComponent<SourceComponent>();
  {
    sfx->setFollowNode(true);
    if (sfx->loadMusicFromFile("Game/resources/sfx.mp3")) {
      sfx->setVolume(200.f);
    } else {
      std::cout << "[Audio] Failed to load sfx.mp3\n";
    }
    moonSfx = sfx;
  }

  // InputSystem: Example of "Mapping Mode", you create a "Mapping", which
  // contains a "Map", a map contains "Actions", an action contains "Bindings",
  // a binding defines what inputs trigger the action, it also has an
  // "Interaction" type, and "Processors", an interaction defines the conditions
  // of the input to trigger the action, and the processor is a modification to
  // the input value before paasing it to the callback.
  // Demo: Jump (tap), Crouch (hold), Move (passthrough Vector2, normalized).
  Mapping* controls = InputSystem::instance().createMapping("DefaultControls");
  ActionMap* gameplay = controls->addMap("Gameplay");

  InputAction* jump = gameplay->addAction("Jump", ActionValueType::kButton);
  jump->addBinding(InputControl{DeviceType::kKeyboard, Key::kSpace, -1, false});
  Interaction tap;
  tap.m_type = InteractionType::kTap;
  tap.m_duration = 0.2f;
  jump->setInteraction(tap);

  InputAction* crouch = gameplay->addAction("Crouch", ActionValueType::kButton);
  crouch->addBinding(InputControl{DeviceType::kKeyboard, Key::kLControl, -1, false});
  Interaction hold;
  hold.m_type = InteractionType::kHold;
  hold.m_duration = 0.4f;
  crouch->setInteraction(hold);

  InputAction* move = gameplay->addAction("Move", ActionValueType::kAxis2D);
  CompositeBinding& moveComposite = move->addComposite(CompositeType::kVector2D);
  moveComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, Key::kA, -1, false}, CompositeRole::kNegativeX, {}});
  moveComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, Key::kD, -1, false}, CompositeRole::kPositiveX, {}});
  moveComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, Key::kS, -1, false}, CompositeRole::kNegativeY, {}});
  moveComposite.m_parts.push_back(
    {InputControl{DeviceType::kKeyboard, Key::kW, -1, false}, CompositeRole::kPositiveY, {}});
  move->addProcessor(Processor{ProcessorType::kNormalize, {}, {}});

  InputSystem::instance().setActiveMapping(controls);

  HEvent jumpSub = jump->onPerformed([](const InputContext&) {
    std::cout << "[Action] Jump performed\n";
  });
  HEvent crouchSub = crouch->onPerformed([](const InputContext&) {
    std::cout << "[Action] Crouch performed (held past threshold)\n";
  });
  HEvent crouchEnd = crouch->onCanceled([](const InputContext&) {
    std::cout << "[Action] Crouch canceled\n";
  });
  float moveReportTimer = 0.f;
  HEvent moveSub = move->onPerformed([&moveReportTimer](const InputContext& ctx) {
    // Performed fires every non-zero frame; throttle the print.
    moveReportTimer += ctx.m_deltaTime;
    if (moveReportTimer >= 0.5f) {
      const Vector2f value = ctx.m_value.asVector2();
      std::cout << "[Action] Move (" << value.x << ", " << value.y << ")\n";
      moveReportTimer = 0.f;
    }
  });
  
  SceneNode* chinese = scene.createNode("Chinese");
  chinese->transform().setPosition({0, center.y});
  auto* cgm = chinese->addComponent<SourceComponent>();
  {
    cgm->setFollowNode(true);
    if (cgm->loadMusicFromFile("Game/resources/chinese.mp3")) {
      cgm->setLooping(true);
      cgm->setVolume(10.f);
      cgm->setMinDistance(50.f);
      cgm->setAttenuation(0.3f);
      cgm->setSpatializationEnabled(true);
      cgm->play();
    } else {
      std::cout << "[Audio] Failed to load chinese.mp3\n";
    }
  }
  SceneNode* mozart = scene.createNode("Mozart");
  mozart->transform().setPosition({static_cast<float>(windowWidth), center.y});
  auto* mgm = mozart->addComponent<SourceComponent>();
  {
    mgm->setFollowNode(true);
    if (mgm->loadMusicFromFile("Game/resources/mozart.mp3")) {
      mgm->setLooping(true);
      mgm->setVolume(10.f);
      mgm->setPitch(2.0f);
      mgm->setMinDistance(50.f);
      mgm->setAttenuation(0.3f);
      mgm->setSpatializationEnabled(true);
      mgm->play();
    } else {
      std::cout << "[Audio] Failed to load mozart.mp3\n";
    }
  }

  SceneNode* neptune = scene.createNode("Neptune", sun2);
  neptune->transform().setPosition({280.f, 100.f});
  neptune->addComponent<CircleComponent>(12.f, sf::Color(80, 100, 200));
  neptune->addComponent<ListenerComponent>();

  sf::Clock clock;
  std::cout << Random::get<float>() << std::endl;
  std::cout << Random::get<float>() << std::endl;
  std::cout << Random::get<float>() << std::endl;
  std::cout << Random::range<int>(0, 30) << std::endl;
  std::cout << Random::range<int>(0, 30) << std::endl;
  std::cout << Random::range<int>(0, 30) << std::endl;
  std::cout << Random::diceThrow(3, 6) << std::endl;
  std::cout << Random::diceThrow(2, 6) << std::endl;
  std::cout << Random::diceThrow(1, 6) << std::endl;

  while (window.isOpen())
  {
    InputSystem::instance().beginFrame();  // snapshot device state before polling

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
      moonSfx->stop();
      moonSfx->play();
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
    sun->transform().rotate(sf::degrees(45.f * deltaTime));
    sun2->transform().rotate(sf::degrees(10.f * deltaTime));
    earth->transform().rotate(sf::degrees(215.f * deltaTime));
    neptune->transform().rotate(sf::degrees(-1.f * deltaTime));

    scene.update(deltaTime);

    window.clear(sf::Color(24, 24, 28));
    scene.draw(window);
    window.display();
  }

  InputSystem::shutDown();

  return 0;
}
