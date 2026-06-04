#include <SFML/Graphics.hpp>

#include "config/IniFile.h"
#include "test.h"
#include "utils/Module.h"

using namespace sfmx;

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

  sfmx::TestComponent testComponent;

  while (window.isOpen())
  {
    while (const Optional<sf::Event> event = window.pollEvent())
    {
      if (event->is<sf::Event::Closed>())
      {
        window.close();
      }
    }

    window.clear(sf::Color(24, 24, 28));
    testComponent.draw(window);
    window.display();
  }

  return 0;
}
