#include "scripts/RegisterIntRect.h"

#include <SFML/Graphics/Rect.hpp>

#include "core/platform/Prerequisites.h"

namespace sfmx
{

namespace script
{

void
registerIntRect(sol::state_view lua) {
  lua.new_usertype<sf::IntRect>("IntRect",
    sol::call_constructor,
    sol::constructors<sf::IntRect(),
                      sf::IntRect(sf::Vector2i, sf::Vector2i)>(),

    "contains", &sf::IntRect::contains,
    "findIntersection", &sf::IntRect::findIntersection,
    "getCenter", &sf::IntRect::getCenter,

    sol::meta_function::equal_to,
    [](const sf::IntRect& left, const sf::IntRect& right) {
      return left == right;
    },

    "position", &sf::IntRect::position,
    "size", &sf::IntRect::size
  );
}

}  // namespace script

}  // namespace sfmx