#include <SFML/Graphics.hpp>

#include "config/IniFile.h"
#include "scene/Scene.h"
#include "utils/Module.h"

using namespace sfmx;

namespace
{
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

 private:
  sf::CircleShape m_circle;
};

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

  SceneNode* sun = scene.createNode("Sun");
  sun->transform().setPosition(
      {static_cast<float>(windowWidth) * 0.5f,
       static_cast<float>(windowHeight) * 0.5f});
  sun->addComponent<CircleComponent>(40.f, sf::Color(255, 180, 100));

  SceneNode* earth = scene.createNode("Earth", sun);
  earth->transform().setPosition({140.f, 0.f});
  earth->addComponent<CircleComponent>(20.f, sf::Color(100, 180, 255));

  SceneNode* moon = scene.createNode("Moon", earth);
  moon->transform().setPosition({40.f, 0.f});
  moon->addComponent<CircleComponent>(4.f, sf::Color(100, 100, 100));

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
