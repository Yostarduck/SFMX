#include "scripts/RegisterTransformComponent.h"

#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/Transform.h"

namespace sfmx
{

namespace script
{

void
registerTransformComponent(sol::state_view lua) {
  lua.new_usertype<Transform>("TransformComponent",
    sol::no_constructor,
    sol::base_classes, sol::bases<Component>(),

    "setPosition", &Transform::setPosition,
    // Scalar overload lets hot scripts move without allocating a Vector2f
    // userdata each frame: transform:move(dx, dy).
    "move", sol::overload(
      [](Transform& t, const sf::Vector2f& offset) { t.move(offset); },
      [](Transform& t, float dx, float dy) { t.move(sf::Vector2f(dx, dy)); }
    ),

    "setRotation", &Transform::setRotation,
    "rotate", &Transform::rotate,

    "setScale", &Transform::setScale,
    "scale", &Transform::scale,

    "getPosition", &Transform::getPosition,
    "getRotation", &Transform::getRotation,
    "getScale", &Transform::getScale,

    "getLocalTransform", &Transform::getLocalTransform,
    "getWorldTransform", &Transform::getWorldTransform
  );
}

}  // namespace script

}  // namespace sfmx