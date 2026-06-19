#include "core/platform/Prerequisites.h"
#include "scripts/registerAngle.h"

#include <SFML/System/Angle.hpp>

#include <functional>

namespace sfmx
{

namespace script
{

void
RegisterAngle(sol::state_view lua) {
  lua.new_usertype<sf::Angle>("Angle",
    sol::call_constructor,
    sol::constructors<sf::Angle()>(),

    "asDegrees", &sf::Angle::asDegrees,
    "asRadians", &sf::Angle::asRadians,
    "wrapSigned", &sf::Angle::wrapSigned,
    "wrapUnsigned", &sf::Angle::wrapUnsigned,

    sol::meta_function::equal_to,
    [](const sf::Angle& left, const sf::Angle& right) {
      return left == right;
    },

    sol::meta_function::less_than,
    [](const sf::Angle& left, const sf::Angle& right) {
      return left < right;
    },

    sol::meta_function::less_than_or_equal_to,
    [](const sf::Angle& left, const sf::Angle& right) {
      return left <= right;
    },

    sol::meta_function::unary_minus,
    [](const sf::Angle& angle) { return -angle; },

    sol::meta_function::addition,
    [](const sf::Angle& left, const sf::Angle& right) {
      return left + right;
    },

    sol::meta_function::subtraction,
    [](const sf::Angle& left, const sf::Angle& right) {
      return left - right;
    },

    sol::meta_function::multiplication,
    sol::overload(
      [](const sf::Angle& left, float right) { return left * right; },
      [](float left, const sf::Angle& right) { return left * right; }
    ),

    sol::meta_function::division,
    sol::overload(
      [](const sf::Angle& left, float right) { return left / right; },
      [](const sf::Angle& left, const sf::Angle& right) {
        return left / right;
      }
    ),

    sol::meta_function::modulus,
    [](const sf::Angle& left, const sf::Angle& right) {
      return left % right;
    },

    "Zero", sol::var(std::cref(sf::Angle::Zero))
  );

  sol::table angle = lua["Angle"];
  angle.set_function("degrees", &sf::degrees);
  angle.set_function("radians", &sf::radians);
}

}

}