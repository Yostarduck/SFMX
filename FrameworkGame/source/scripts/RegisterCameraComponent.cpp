#include "scripts/RegisterCameraComponent.h"

#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/CameraComponent.h"

namespace sfmx
{

namespace script
{

void
registerCameraComponent(sol::state_view lua) {
  lua.new_usertype<CameraComponent>("CameraComponent",
    sol::no_constructor,
    sol::base_classes, sol::bases<Component>(),

    // Stamps the concrete type id onto the usertype so Lua can drive
    // node:addComponent(CameraComponent, ...) etc. (see registerComponentAccess).
    "typeId", sol::var(componentTypeId<CameraComponent>()),

    "setViewport", &CameraComponent::setViewport,
    "getViewport", &CameraComponent::getViewport,

    "move", &CameraComponent::move,
    "setCenter", &CameraComponent::setCenter,
    "getCenter", &CameraComponent::getCenter,

    "rotate", sol::overload(
      [](CameraComponent& c, float deltaDegrees) {
        c.rotate(deltaDegrees);
      },
      [](CameraComponent& c, const sf::Angle& angle) {
        c.rotate(angle);
      }
    ),
    "setRotation", sol::overload(
      [](CameraComponent& c, const sf::Angle& angle) {
        c.setRotation(angle);
      },
      [](CameraComponent& c, float degrees) {
        c.setRotation(degrees);
      }
    ),
    "getRotation", &CameraComponent::getRotation,
    "getRotationDegrees", &CameraComponent::getRotationDegrees,

    "setSize", &CameraComponent::setSize,
    "getSize", &CameraComponent::getSize,

    "zoom", &CameraComponent::zoom,

    "getTransform", &CameraComponent::getTransform,
    "getInverseTransform", &CameraComponent::getInverseTransform,

    "setFollowNode", &CameraComponent::setFollowNode,
    "isFollowingNode", &CameraComponent::isFollowingNode,

    "setDrawOrder", &CameraComponent::setDrawOrder,
    "getDrawOrder", &CameraComponent::getDrawOrder
  );
}

}  // namespace script

}  // namespace sfmx