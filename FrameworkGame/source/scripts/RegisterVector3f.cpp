#include "scripts/RegisterVector3f.h"

#include <SFML/System/Vector3.hpp>

#include "core/platform/Prerequisites.h"

namespace sfmx
{

namespace script
{

void
registerVector3f(sol::state_view lua) {
  lua.new_usertype<sf::Vector3f>("Vector3f",
    sol::call_constructor,
    sol::constructors<sf::Vector3f(), sf::Vector3f(float, float, float)>(),

    "length", &sf::Vector3f::length,
    "lengthSquared", &sf::Vector3f::lengthSquared,
    "normalized", &sf::Vector3f::normalized,
    "dot", &sf::Vector3f::dot,
    "cross", &sf::Vector3f::cross,

    sol::meta_function::unary_minus,
    [](const sf::Vector3f& v) { return -v; },

    sol::meta_function::addition,
    [](const sf::Vector3f& left, const sf::Vector3f& right) {
      return left + right;
    },

    sol::meta_function::subtraction,
    [](const sf::Vector3f& left, const sf::Vector3f& right) {
      return left - right;
    },

    sol::meta_function::multiplication,
    sol::overload(
      [](const sf::Vector3f& left, float right) { return left * right; },
      [](float left, const sf::Vector3f& right) { return left * right; }
    ),

    sol::meta_function::division,
    [](const sf::Vector3f& left, float right) { return left / right; },

    sol::meta_function::equal_to,
    [](const sf::Vector3f& left, const sf::Vector3f& right) {
      return left == right;
    },

    "x", &sf::Vector3f::x,
    "y", &sf::Vector3f::y,
    "z", &sf::Vector3f::z
  );
}

}  // namespace script

}  // namespace sfmx