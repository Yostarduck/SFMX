#include <SFML/Graphics.hpp>

#include "config/IniFile.h"
#include "scene/Scene.h"
#include "scene/ParticleSystemComponent.h"
#include "utils/Module.h"
#include "utils/EventSystem.h"

using namespace sfmx;

/**
 * @brief Minimal drawable component used to smoke-test the scene graph: draws a
 *        circle with the accumulated world transform supplied by the node.
 */
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
  
  void
  onUpdate(float deltaTime) override {
    m_onUpdateEvent(*this, deltaTime);
  }

  NODISCARD HEvent onUpdateEvent(std::function<void(const CircleComponent&, float)> foo) const 
  { return m_onUpdateEvent.connect(foo); }

 private:
  sf::CircleShape m_circle;
  Event<void(const CircleComponent&, float)> m_onUpdateEvent;
};

namespace
{
/**
 * @brief Minimal Module<T> example used to confirm the lifecycle works.
 */
class ExampleModule : public Module<ExampleModule>
{
 public:
  int getValue() const { return m_value; }

 private:
  // Module<ExampleModule> owns construction/destruction of the instance.
  friend class Module<ExampleModule>;

  explicit ExampleModule(int value) : m_value(value) {}

  int m_value = 0;
};

void runModuleExample()
{
  std::cout << "[Module] started before startUp(): "
            << ExampleModule::isStarted() << "\n";

  ExampleModule::startUp(42);
  std::cout << "[Module] started after startUp():  "
            << ExampleModule::isStarted() << "\n";
  std::cout << "[Module] instance().getValue():    "
            << ExampleModule::instance().getValue() << "\n";
  std::cout << "[Module] instancePtr()->getValue():"
            << ExampleModule::instancePtr()->getValue() << "\n";

  ExampleModule::shutDown();
  std::cout << "[Module] started after shutDown():  "
            << ExampleModule::isStarted() << "\n";

  // The module can be restarted after a shut down.
  ExampleModule::startUp(7);
  std::cout << "[Module] restarted value:          "
            << ExampleModule::instance().getValue() << "\n";
  ExampleModule::shutDown();
}
} // namespace

DECLARE_TYPE_TRAITS(CircleComponent)

int main()
{
  runModuleExample();

  sfmx::IniFile config;
  config.loadAll({"Game/config/Engine.ini", "Game/config/Game.ini"});

  const uint32 windowWidth = config.getUInt("Window", "Width", 800u);
  const uint32 windowHeight = config.getUInt("Window", "Height", 600u);
  const String windowTitle = config.getString("Window", "Title", "SFMX Game");
  const bool enableVSync = config.getBool("Window", "VSync", true);

  sf::RenderWindow window(sf::VideoMode({windowWidth, windowHeight}), windowTitle);
  window.setVerticalSyncEnabled(enableVSync);

  /**
   * Small Sun-Earth-Moon hierarchy: a parent circle with an orbiting child
   * circle with an orbiting child circle. 
   **/
  Scene scene("Main", 1024);
  scene.registerComponent<CircleComponent>(64);
  scene.registerComponent<ParticleSystemComponent>(64);
  
  SceneNode* sun = scene.createNode("Sun");
  sun->transform().setPosition(
      {static_cast<float>(windowWidth) * 0.5f,
       static_cast<float>(windowHeight) * 0.5f});
  auto* sunComponent = sun->addComponent<CircleComponent>(40.f, sf::Color(255, 180, 100));

  float totalTime = 0.f;
  auto sunUpdateEvent = sunComponent->onUpdateEvent([&totalTime](const CircleComponent& comp, float deltaTime) {
    totalTime += deltaTime;
    if (totalTime >= 1.f) {
      // Print delta every second to confirm the event system is working and can capture local state.
      std::cout << "[Event] Sun's CircleComponent onUpdate: deltaTime = " << deltaTime << "s\n";
      totalTime = 0.f;
    }
  });

  auto* texture = new sf::Texture();
  if(!texture->loadFromFile("Game/resources/particle.png"))  // or "particle.png" depending on cwd
  {
    std::cerr << "Failed Loading resource" << std::endl; 
    return -1;
  }
  EmitterConfig emitterConfig;
  emitterConfig.emissionRate          = 60.f;
  emitterConfig.maxParticles          = 200;
  emitterConfig.direction             = sf::degrees(-90.f);       // shoot upward
  emitterConfig.directionVariance     = sf::degrees(30.f);        // spread cone
  emitterConfig.speed                 = 200.f;
  emitterConfig.speedVariance         = 50.f;
  emitterConfig.startRotation         = sf::degrees(0.f);
  emitterConfig.startRotationVariance = sf::degrees(180.f);
  emitterConfig.angularVelocity       = 180.f;
  emitterConfig.angularVelocityVariance = 90.f;
  emitterConfig.gravity               = {0.f, 100.f};              // gentle downward pull
  emitterConfig.startColor            = sf::Color(255, 200, 100);  // bright warm
  emitterConfig.endColor              = sf::Color(255, 50, 50, 0); // fade to transparent red
  emitterConfig.startSize             = {32.f, 32.f};
  emitterConfig.endSize               = {4.f, 4.f};
  emitterConfig.lifetime              = 1.5f;
  emitterConfig.lifetimeVariance      = 0.5f;
  emitterConfig.texture               = nullptr;
  emitterConfig.blendMode             = sf::BlendAlpha;
  
  SceneNode* earth = scene.createNode("Earth", sun);
  earth->transform().setPosition({140.f, 0.f});
  earth->addComponent<CircleComponent>(20.f, sf::Color(100, 180, 255));

  SceneNode* moon = scene.createNode("Moon", earth);
  moon->transform().setPosition({40.f, 0.f});
  moon->addComponent<CircleComponent>(4.f, sf::Color(100, 100, 100));

  auto* particles = moon->addComponent<ParticleSystemComponent>();
  particles->setConfig(emitterConfig);
  particles->setSortMode(ParticleSortMode::BackToFront);

  // Burst 50 particles on start
  particles->emit(50);

  float particleTimer = 0.0f;
  sf::Clock clock;
  while (window.isOpen())
  {
    while (const Optional<sf::Event> event = window.pollEvent())
    {
      if (event->is<sf::Event::Closed>())
      {
        window.close();
      }
    }

    const float deltaTime = clock.restart().asSeconds();
    particleTimer += deltaTime;

    if (particleTimer > 5.0f)
    {
      particleTimer = 0.0f;
      particles->emit(10);
      std::cout << "boom" << std::endl;
    }
    // Rotating the parent drags the child with it: proof of transform
    // composition down the hierarchy.
    sun->transform().rotate(sf::degrees(45.f * deltaTime));
    
    earth->transform().rotate(sf::degrees(215.f * deltaTime));

    scene.update(deltaTime);

    window.clear(sf::Color(24, 24, 28));
    scene.draw(window);
    window.display();
  }

  return 0;
}
