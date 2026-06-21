#include "scripts/registerVector3i.h"

#include <SFML/System/Vector3.hpp>

#include "core/platform/Prerequisites.h"

namespace sfmx
{

namespace script
{

void
registerVector3i(sol::state_view lua) {
  lua.new_usertype<sf::Vector3i>("Vector3i",
    sol::call_constructor,
    sol::constructors<sf::Vector3i(), sf::Vector3i(int32, int32, int32)>(),

    "length", [](const sf::Vector3i& v) {
      return sf::Vector3f(v).length();
    },
    "lengthSquared", &sf::Vector3i::lengthSquared,
    "dot", &sf::Vector3i::dot,
    "cross", &sf::Vector3i::cross,

    sol::meta_function::unary_minus,
    [](const sf::Vector3i& v) { return -v; },

    sol::meta_function::addition,
    [](const sf::Vector3i& left, const sf::Vector3i& right) {
      return left + right;
    },

    sol::meta_function::subtraction,
    [](const sf::Vector3i& left, const sf::Vector3i& right) {
      return left - right;
    },

    sol::meta_function::multiplication,
    sol::overload(
      [](const sf::Vector3i& left, int32 right) { return left * right; },
      [](int32 left, const sf::Vector3i& right) { return left * right; }
    ),

    sol::meta_function::division,
    [](const sf::Vector3i& left, int32 right) { return left / right; },

    sol::meta_function::equal_to,
    [](const sf::Vector3i& left, const sf::Vector3i& right) {
      return left == right;
    },

    "x", &sf::Vector3i::x,
    "y", &sf::Vector3i::y,
    "z", &sf::Vector3i::z
  );
}

}  // namespace script

}  // namespace sfmx