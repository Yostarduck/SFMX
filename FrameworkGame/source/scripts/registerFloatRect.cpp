#include "core/platform/Prerequisites.h"
#include "scripts/registerFloatRect.h"

#include <SFML/Graphics/Rect.hpp>

namespace sfmx
{

namespace script
{

void
RegisterFloatRect(sol::state_view lua) {
  lua.new_usertype<sf::FloatRect>("FloatRect",
    sol::call_constructor,
    sol::constructors<sf::FloatRect(),
                      sf::FloatRect(sf::Vector2f, sf::Vector2f)>(),

    "contains", &sf::FloatRect::contains,
    "findIntersection", &sf::FloatRect::findIntersection,
    "getCenter", &sf::FloatRect::getCenter,

    sol::meta_function::equal_to,
    [](const sf::FloatRect& left, const sf::FloatRect& right) {
      return left == right;
    },

    "position", &sf::FloatRect::position,
    "size", &sf::FloatRect::size
  );
}

}

}