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

DECLARE_TYPE_TRAITS(CircleComponent)

int main()
{
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

  // -- Load shared particle texture --
  auto* texture = new sf::Texture();
  if (!texture->loadFromFile("Game/resources/particle.png"))
  {
    std::cerr << "Failed loading particle texture\n";
    delete texture;
    return -1;
  }

  // -- Emitter 1: looping fountain (auto-restarts) --
  SceneNode* left = scene.createNode("LoopingEmitter");
  left->transform().setPosition({windowWidth * 0.3f, windowHeight * 0.6f});

  EmitterConfig loopCfg;
  loopCfg.emissionRate          = 60.f;
  loopCfg.maxParticles          = 200;
  loopCfg.direction             = sf::degrees(-90.f);
  loopCfg.directionVariance     = sf::degrees(25.f);
  loopCfg.speed                 = 250.f;
  loopCfg.speedVariance         = 50.f;
  loopCfg.startRotation         = sf::degrees(0.f);
  loopCfg.startRotationVariance = sf::degrees(360.f);
  loopCfg.angularVelocity       = 200.f;
  loopCfg.angularVelocityVariance = 100.f;
  loopCfg.gravity               = {0.f, 200.f};
  loopCfg.startColor            = sf::Color(100, 200, 255);
  loopCfg.endColor              = sf::Color(100, 200, 255, 0);
  loopCfg.startSize             = {24.f, 24.f};
  loopCfg.endSize               = {2.f, 2.f};
  loopCfg.lifetime              = 2.f;
  loopCfg.lifetimeVariance      = 0.5f;
  loopCfg.texture               = texture;
  loopCfg.blendMode             = sf::BlendAlpha;
  loopCfg.duration              = 2.5f;
  loopCfg.loop                  = true;

  auto* loopEmitter = left->addComponent<ParticleSystemComponent>();
  loopEmitter->setConfig(loopCfg);
  loopEmitter->setSortMode(ParticleSortMode::BackToFront);
  loopEmitter->setWorldSpace(true);

  // -- Emitter 2: burst on Space key --
  SceneNode* right = scene.createNode("BurstEmitter");
  right->transform().setPosition({windowWidth * 0.7f, windowHeight * 0.6f});

  EmitterConfig burstCfg;
  burstCfg.emissionRate          = 0.f;     // manual burst only
  burstCfg.maxParticles          = 100;
  burstCfg.direction             = sf::degrees(-90.f);
  burstCfg.directionVariance     = sf::degrees(45.f);
  burstCfg.speed                 = 300.f;
  burstCfg.speedVariance         = 100.f;
  burstCfg.startRotation         = sf::degrees(0.f);
  burstCfg.startRotationVariance = sf::degrees(360.f);
  burstCfg.angularVelocity       = 180.f;
  burstCfg.angularVelocityVariance = 90.f;
  burstCfg.gravity               = {0.f, 150.f};
  burstCfg.startColor            = sf::Color(255, 180, 100);
  burstCfg.endColor              = sf::Color(255, 50, 50, 0);
  burstCfg.startSize             = {28.f, 28.f};
  burstCfg.endSize               = {2.f, 2.f};
  burstCfg.lifetime              = 2.f;
  burstCfg.lifetimeVariance      = 0.5f;
  burstCfg.texture               = texture;
  burstCfg.blendMode             = sf::BlendAlpha;
  burstCfg.duration              = 0.f;
  burstCfg.loop                  = false;

  auto* burstEmitter = right->addComponent<ParticleSystemComponent>();
  burstEmitter->setConfig(burstCfg);
  burstEmitter->setSortMode(ParticleSortMode::BackToFront);
  burstEmitter->setWorldSpace(true);

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
          burstEmitter->emit(50);
          std::cout << "[Burst] 50 particles emitted ("
                    << burstEmitter->getParticleCount() << " alive)\n";
        }
        if (key->code == sf::Keyboard::Key::L)
        {
          loopEmitter->start();
          std::cout << "[Loop] restarting\n";
        }
      }
    }

    const float deltaTime = clock.restart().asSeconds();

    scene.update(deltaTime);

    window.clear(sf::Color(24, 24, 28));
    scene.draw(window);
    window.display();
  }

  return 0;
}
