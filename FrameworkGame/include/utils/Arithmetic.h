#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"

namespace sfmx::lerp
{

/** @brief lerps a number from a min value to b max value by t percentage */
NODISCARD inline float
number(float a, float b, float t) {
  return (a * (1.0 - t)) + (b * t);
}

/** @brief Generates a lerped vector between two points */
NODISCARD inline sf::Vector2f
vector2(const sf::Vector2f& a, const sf::Vector2f& b, float t) {
  return {lerp::number(a.x, b.x, t), lerp::number(a.y, b.y, t)};
}

/** @brief lerps a Color into another with a simple system */
NODISCARD inline sf::Color
color(const sf::Color& a, const sf::Color& b, float t) {
  return sf::Color(static_cast<uint8>(lerp::number(static_cast<float>(a.r),
                                                   static_cast<float>(b.r), t)),
                   static_cast<uint8>(lerp::number(static_cast<float>(a.g),
                                                   static_cast<float>(b.g), t)),
                   static_cast<uint8>(lerp::number(static_cast<float>(a.b),
                                                   static_cast<float>(b.b), t)),
                   static_cast<uint8>(lerp::number(static_cast<float>(a.a),
                                                   static_cast<float>(b.a), t))
  );
}

} // namespace sfmx::lerp
