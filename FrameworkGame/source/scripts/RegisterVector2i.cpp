#include "scripts/RegisterVector2i.h"

#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"

namespace sfmx
{

namespace script
{

void
registerVector2i(sol::state_view lua) {
  lua.new_usertype<sf::Vector2i>("Vector2i",
    sol::call_constructor,
    sol::constructors<sf::Vector2i(), sf::Vector2i(int32, int32)>(),

    "length", [](const sf::Vector2i& v) {
      return sf::Vector2f(v).length();
    },
    "lengthSquared", &sf::Vector2i::lengthSquared,
    "perpendicular", &sf::Vector2i::perpendicular,
    "dot", &sf::Vector2i::dot,
    "cross", &sf::Vector2i::cross,

    sol::meta_function::unary_minus,
    [](const sf::Vector2i& v) { return -v; },

    sol::meta_function::addition,
    [](const sf::Vector2i& left, const sf::Vector2i& right) {
      return left + right;
    },

    sol::meta_function::subtraction,
    [](const sf::Vector2i& left, const sf::Vector2i& right) {
      return left - right;
    },

    sol::meta_function::multiplication,
    sol::overload(
      [](const sf::Vector2i& left, int32 right) { return left * right; },
      [](int32 left, const sf::Vector2i& right) { return left * right; }
    ),

    sol::meta_function::division,
    [](const sf::Vector2i& left, int32 right) { return left / right; },

    sol::meta_function::equal_to,
    [](const sf::Vector2i& left, const sf::Vector2i& right) {
      return left == right;
    },

    "x", &sf::Vector2i::x,
    "y", &sf::Vector2i::y
  );
}

}  // namespace script

}  // namespace sfmx