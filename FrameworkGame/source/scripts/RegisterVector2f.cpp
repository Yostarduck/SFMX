#include "scripts/RegisterVector2f.h"

#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"

namespace sfmx
{

namespace script
{

void
registerVector2f(sol::state_view lua) {
  lua.new_usertype<sf::Vector2f>("Vector2f",
    sol::call_constructor,
    sol::constructors<sf::Vector2f(), sf::Vector2f(float, float)>(),

    "length", &sf::Vector2f::length,
    "lengthSquared", &sf::Vector2f::lengthSquared,
    "normalized", &sf::Vector2f::normalized,
    "angleTo", &sf::Vector2f::angleTo,
    "angle", &sf::Vector2f::angle,
    "rotatedBy", &sf::Vector2f::rotatedBy,
    "projectedOnto", &sf::Vector2f::projectedOnto,
    "perpendicular", &sf::Vector2f::perpendicular,
    "dot", &sf::Vector2f::dot,
    "cross", &sf::Vector2f::cross,

    sol::meta_function::unary_minus,
    [](const sf::Vector2f& v) { return -v; },

    sol::meta_function::addition,
    [](const sf::Vector2f& left, const sf::Vector2f& right) {
      return left + right;
    },

    sol::meta_function::subtraction,
    [](const sf::Vector2f& left, const sf::Vector2f& right) {
      return left - right;
    },

    sol::meta_function::multiplication,
    sol::overload(
      [](const sf::Vector2f& left, float right) { return left * right; },
      [](float left, const sf::Vector2f& right) { return left * right; }
    ),

    sol::meta_function::division,
    [](const sf::Vector2f& left, float right) { return left / right; },

    sol::meta_function::equal_to,
    [](const sf::Vector2f& left, const sf::Vector2f& right) {
      return left == right;
    },

    "x", &sf::Vector2f::x,
    "y", &sf::Vector2f::y
  );
}

}  // namespace script

}  // namespace sfmx