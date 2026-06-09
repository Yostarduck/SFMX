#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"

namespace sfmx::lerp
{

NODISCARD inline sf::Vector2f
vector2(const sf::Vector2f& a, const sf::Vector2f& b, float t)
{
  return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

NODISCARD inline sf::Color
color(const sf::Color& a, const sf::Color& b, float t)
{
  return sf::Color(
    static_cast<uint8>(static_cast<float>(a.r) +
      (static_cast<float>(b.r) - static_cast<float>(a.r)) * t),
    static_cast<uint8>(static_cast<float>(a.g) +
      (static_cast<float>(b.g) - static_cast<float>(a.g)) * t),
    static_cast<uint8>(static_cast<float>(a.b) +
      (static_cast<float>(b.b) - static_cast<float>(a.b)) * t),
    static_cast<uint8>(static_cast<float>(a.a) +
      (static_cast<float>(b.a) - static_cast<float>(a.a)) * t)
  );
}

} // namespace sfmx::lerp
