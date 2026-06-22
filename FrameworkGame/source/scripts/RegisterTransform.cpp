#include "scripts/RegisterTransform.h"

#include <SFML/System/Angle.hpp>
#include <SFML/Graphics/Transform.hpp>

#include <algorithm>

#include "core/platform/Prerequisites.h"

namespace sfmx
{

namespace script
{

void
registerTransform(sol::state_view lua) {
  lua.new_usertype<sf::Transform>("Transform",
    sol::call_constructor,
    sol::constructors<sf::Transform(),
                      sf::Transform(float, float, float,
                                    float, float, float,
                                    float, float, float)>(),

    "getInverse", &sf::Transform::getInverse,
    "transformPoint", &sf::Transform::transformPoint,
    "transformRect", &sf::Transform::transformRect,
    "combine", &sf::Transform::combine,
    "translate", &sf::Transform::translate,
    "rotate", sol::overload(
      [](sf::Transform& t, sf::Angle angle) -> sf::Transform& {
        return t.rotate(angle);
      },
      [](sf::Transform& t, sf::Angle angle, sf::Vector2f center) -> sf::Transform& {
        return t.rotate(angle, center);
      }
    ),
    "scale", sol::overload(
      [](sf::Transform& t, sf::Vector2f factors) -> sf::Transform& {
        return t.scale(factors);
      },
      [](sf::Transform& t, sf::Vector2f factors, sf::Vector2f center) -> sf::Transform& {
        return t.scale(factors, center);
      }
    ),

    "Identity", sol::var(std::cref(sf::Transform::Identity)),

    sol::meta_function::multiplication,
    sol::overload(
      [](const sf::Transform& left, const sf::Transform& right) {
        return left * right;
      },
      [](const sf::Transform& left, sf::Vector2f right) {
        return left * right;
      }
    ),

    sol::meta_function::equal_to,
    [](const sf::Transform& left, const sf::Transform& right) {
      return left == right;
    }
  );
}

}  // namespace script

}  // namespace sfmx