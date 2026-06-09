#include <SFML/Graphics.hpp>

#include "config/IniFile.h"
#include "scene/ListenerComponent.h"
#include "scene/Scene.h"
#include "scene/SourceComponent.h"

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
  while (window.isOpen())
  {
    while (const Optional<sf::Event> event = window.pollEvent())
    {
      if (event->is<sf::Event::Closed>())
      {
        window.close();
      }
      if (const auto* key = event->getIf<sf::Event::KeyPressed>())
      {
        if (key->code == sf::Keyboard::Key::Space && moonSfx)
        {
          moonSfx->stop();
          moonSfx->play();
        }
      }
    }

    const float deltaTime = clock.restart().asSeconds();

    sun->transform().rotate(sf::degrees(45.f * deltaTime));
    sun2->transform().rotate(sf::degrees(10.f * deltaTime));
    earth->transform().rotate(sf::degrees(215.f * deltaTime));
    neptune->transform().rotate(sf::degrees(-1.f * deltaTime));

    scene.update(deltaTime);

    window.clear(sf::Color(24, 24, 28));
    scene.draw(window);
    window.display();
  }

  return 0;
}
