#include "scripts/RegisterColor.h"

#include <SFML/Graphics/Color.hpp>

#include <algorithm>
#include <functional>

#include "core/platform/Prerequisites.h"

namespace sfmx
{

namespace script
{

void
registerColor(sol::state_view lua) {
  lua.new_usertype<sf::Color>("Color",
    sol::call_constructor,
    sol::constructors<
      sf::Color(),
      sf::Color(uint8, uint8, uint8),
      sf::Color(uint8, uint8, uint8, uint8),
      sf::Color(uint32)
    >(),

    "toInteger", &sf::Color::toInteger,

    "Black", sol::var(std::cref(sf::Color::Black)),
    "White", sol::var(std::cref(sf::Color::White)),
    "Red", sol::var(std::cref(sf::Color::Red)),
    "Green", sol::var(std::cref(sf::Color::Green)),
    "Blue", sol::var(std::cref(sf::Color::Blue)),
    "Yellow", sol::var(std::cref(sf::Color::Yellow)),
    "Magenta", sol::var(std::cref(sf::Color::Magenta)),
    "Cyan", sol::var(std::cref(sf::Color::Cyan)),
    "Transparent", sol::var(std::cref(sf::Color::Transparent)),

    sol::meta_function::addition,
    [](const sf::Color& left, const sf::Color& right) {
      return left + right;
    },

    sol::meta_function::subtraction,
    [](const sf::Color& left, const sf::Color& right) {
      return left - right;
    },

    sol::meta_function::multiplication,
    [](const sf::Color& left, const sf::Color& right) {
      return left * right;
    },

    sol::meta_function::equal_to,
    [](const sf::Color& left, const sf::Color& right) {
      return left == right;
    },

    "r", sol::property(
      [](const sf::Color& c) { return c.r; },
      [](sf::Color& c, int32 value) {
        c.r = static_cast<uint8>(std::clamp(value, 0, 255));
      }
    ),
    "g", sol::property(
      [](const sf::Color& c) { return c.g; },
      [](sf::Color& c, int32 value) {
        c.g = static_cast<uint8>(std::clamp(value, 0, 255));
      }
    ),
    "b", sol::property(
      [](const sf::Color& c) { return c.b; },
      [](sf::Color& c, int32 value) {
        c.b = static_cast<uint8>(std::clamp(value, 0, 255));
      }
    ),
    "a", sol::property(
      [](const sf::Color& c) { return c.a; },
      [](sf::Color& c, int32 value) {
        c.a = static_cast<uint8>(std::clamp(value, 0, 255));
      }
    )
  );
}

}  // namespace script

}  // namespace sfmx