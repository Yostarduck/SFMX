#pragma once

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace sfmx
{
class TestComponent
{
public:
  TestComponent();
  void draw(sf::RenderTarget& target) const;

private:
  sf::CircleShape m_circle;
};
}
