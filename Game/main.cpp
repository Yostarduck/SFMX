#include <SFML/Graphics.hpp>
#include "test.h"

int main()
{
  sf::RenderWindow window(sf::VideoMode({800u, 600u}), "SFMX Game");
  sfmx::TestComponent testComponent;

  while (window.isOpen())
  {
    while (const std::optional event = window.pollEvent())
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
