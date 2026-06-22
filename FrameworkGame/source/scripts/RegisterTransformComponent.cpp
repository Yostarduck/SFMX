#include "scripts/RegisterTransformComponent.h"

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
    "move", &Transform::move,

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