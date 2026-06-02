#include <SFML/Graphics.hpp>

#include "config/IniFile.h"
#include "test.h"

using namespace sfmx;

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
