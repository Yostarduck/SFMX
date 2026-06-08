#include <SFML/Graphics.hpp>

#include "config/IniFile.h"
#include "scene/Scene.h"
#include "scene/ParticleSystemComponent.h"
#include "utils/Module.h"
#include "utils/EventSystem.h"

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

class ExampleModule : public Module<ExampleModule>
{
 public:
  int getValue() const { return m_value; }

 private:
  friend class Module<ExampleModule>;
  explicit ExampleModule(int value) : m_value(value) {}
  int m_value = 0;
};

void runModuleExample()
{
  std::cout << "[Module] started before startUp(): " << ExampleModule::isStarted() << "\n";
  ExampleModule::startUp(42);
  std::cout << "[Module] started after startUp():  " << ExampleModule::isStarted() << "\n";
  std::cout << "[Module] instance().getValue():    " << ExampleModule::instance().getValue() << "\n";
  std::cout << "[Module] instancePtr()->getValue():" << ExampleModule::instancePtr()->getValue() << "\n";
  ExampleModule::shutDown();
  ExampleModule::startUp(7);
  std::cout << "[Module] restarted value:          " << ExampleModule::instance().getValue() << "\n";
  ExampleModule::shutDown();
}
} // namespace

DECLARE_TYPE_TRAITS(CircleComponent)

int main()
{
  runModuleExample();

  sfmx::IniFile config;
  config.loadAll({"Game/config/Engine.ini", "Game/config/Game.ini"});

  const uint32 windowWidth  = config.getUInt("Window", "Width", 800u);
  const uint32 windowHeight = config.getUInt("Window", "Height", 600u);
  const String windowTitle  = config.getString("Window", "Title", "SFMX Game");
  const bool enableVSync    = config.getBool("Window", "VSync", true);

  sf::RenderWindow window(sf::VideoMode({windowWidth, windowHeight}), windowTitle);
  window.setVerticalSyncEnabled(enableVSync);

  Scene scene("Main", 1024);
  scene.registerComponent<CircleComponent>(64);
  scene.registerComponent<ParticleSystemComponent>(4);

  SceneNode* sun = scene.createNode("Sun");
  sun->transform().setPosition(
      {static_cast<float>(windowWidth) * 0.5f,
       static_cast<float>(windowHeight) * 0.5f});
  auto* sunComponent = sun->addComponent<CircleComponent>(40.f, sf::Color(255, 180, 100));

  float totalTime = 0.f;
  auto sunUpdateEvent = sunComponent->onUpdateEvent([&totalTime](const CircleComponent&, float deltaTime) {
    totalTime += deltaTime;
    if (totalTime >= 1.f) {
      std::cout << "[Event] Sun's CircleComponent onUpdate: deltaTime = " << deltaTime << "s\n";
      totalTime = 0.f;
    }
  });

  auto* texture = new sf::Texture();
  if (!texture->loadFromFile("Game/resources/particle.png"))
  {
    std::cerr << "Failed loading particle texture\n";
    delete texture;
    return -1;
  }

  // -- Sun: fiery corona (local space, follows the sun's rotation) --
  EmitterConfig sunCfg;
  sunCfg.emissionRate          = 40.f;
  sunCfg.maxParticles          = 150;
  sunCfg.direction             = sf::degrees(0.f);
  sunCfg.directionVariance     = sf::degrees(360.f);
  sunCfg.speed                 = 120.f;
  sunCfg.speedVariance         = 40.f;
  sunCfg.startRotation         = sf::degrees(0.f);
  sunCfg.startRotationVariance = sf::degrees(360.f);
  sunCfg.angularVelocity       = 90.f;
  sunCfg.angularVelocityVariance = 45.f;
  sunCfg.gravity               = {-40.f, 0.f};
  sunCfg.startColor            = sf::Color(255, 200, 80);
  sunCfg.endColor              = sf::Color(255, 50, 0, 0);
  sunCfg.startSize             = {20.f, 20.f};
  sunCfg.endSize               = {0.f, 0.f};
  sunCfg.lifetime              = 1.5f;
  sunCfg.lifetimeVariance      = 0.5f;
  sunCfg.texture               = texture;
  sunCfg.blendMode             = sf::BlendAlpha;
  sunCfg.duration              = 0.f;
  sunCfg.loop                  = false;

  auto* sunParticles = sun->addComponent<ParticleSystemComponent>();
  sunParticles->setConfig(sunCfg);
  sunParticles->setSortMode(ParticleSortMode::BackToFront);
  sunParticles->setWorldSpace(false);

  SceneNode* earth = scene.createNode("Earth", sun);
  earth->transform().setPosition({140.f, 0.f});
  earth->addComponent<CircleComponent>(20.f, sf::Color(100, 180, 255));

  // -- Earth: atmospheric glow (world space, trails as it orbits) --
  EmitterConfig earthCfg;
  earthCfg.emissionRate          = 20.f;
  earthCfg.maxParticles          = 80;
  earthCfg.direction             = sf::degrees(0.f);
  earthCfg.directionVariance     = sf::degrees(360.f);
  earthCfg.speed                 = 30.f;
  earthCfg.speedVariance         = 10.f;
  earthCfg.startRotation         = sf::degrees(0.f);
  earthCfg.startRotationVariance = sf::degrees(360.f);
  earthCfg.angularVelocity       = 30.f;
  earthCfg.angularVelocityVariance = 15.f;
  earthCfg.gravity               = {0.f, 0.f};
  earthCfg.startColor            = sf::Color(100, 200, 255, 180);
  earthCfg.endColor              = sf::Color(100, 200, 255, 0);
  earthCfg.startSize             = {16.f, 16.f};
  earthCfg.endSize               = {0.f, 0.f};
  earthCfg.lifetime              = 1.0f;
  earthCfg.lifetimeVariance      = 0.3f;
  earthCfg.texture               = texture;
  earthCfg.blendMode             = sf::BlendAlpha;
  earthCfg.duration              = 0.f;
  earthCfg.loop                  = false;

  auto* earthParticles = earth->addComponent<ParticleSystemComponent>();
  earthParticles->setConfig(earthCfg);
  earthParticles->setSortMode(ParticleSortMode::BackToFront);
  earthParticles->setWorldSpace(true);

  SceneNode* moon = scene.createNode("Moon", earth);
  moon->transform().setPosition({40.f, 0.f});
  moon->addComponent<CircleComponent>(4.f, sf::Color(100, 100, 100));

  // -- Moon: dust burst on Space --
  EmitterConfig moonCfg;
  moonCfg.emissionRate          = 0.f;
  moonCfg.maxParticles          = 50;
  moonCfg.direction             = sf::degrees(-90.f);
  moonCfg.directionVariance     = sf::degrees(45.f);
  moonCfg.speed                 = 150.f;
  moonCfg.speedVariance         = 50.f;
  moonCfg.startRotation         = sf::degrees(0.f);
  moonCfg.startRotationVariance = sf::degrees(360.f);
  moonCfg.angularVelocity       = 200.f;
  moonCfg.angularVelocityVariance = 100.f;
  moonCfg.gravity               = {0.f, 100.f};
  moonCfg.startColor            = sf::Color(200, 200, 200);
  moonCfg.endColor              = sf::Color(100, 100, 255, 0);
  moonCfg.startSize             = {12.f, 12.f};
  moonCfg.endSize               = {2.f, 2.f};
  moonCfg.lifetime              = 1.5f;
  moonCfg.lifetimeVariance      = 0.5f;
  moonCfg.texture               = texture;
  moonCfg.blendMode             = sf::BlendAlpha;
  moonCfg.duration              = 0.f;
  moonCfg.loop                  = false;

  auto* moonParticles = moon->addComponent<ParticleSystemComponent>();
  moonParticles->setConfig(moonCfg);
  moonParticles->setSortMode(ParticleSortMode::BackToFront);
  moonParticles->setWorldSpace(true);

  sf::Clock clock;
  while (window.isOpen())
  {
    while (const Optional<sf::Event> event = window.pollEvent())
    {
      if (event->is<sf::Event::Closed>())
      {
        window.close();
      }
      if (auto* key = event->getIf<sf::Event::KeyPressed>())
      {
        if (key->code == sf::Keyboard::Key::Space)
        {
          moonParticles->emit(30);
          std::cout << "[Moon] Burst 30 particles\n";
        }
      }
    }

    const float deltaTime = clock.restart().asSeconds();

    sun->transform().rotate(sf::degrees(45.f * deltaTime));
    earth->transform().rotate(sf::degrees(215.f * deltaTime));

    scene.update(deltaTime);

    window.clear(sf::Color(24, 24, 28));
    scene.draw(window);
    window.display();
  }

  return 0;
}
