#include "scripts/RegisterListenerComponent.h"

#include "core/platform/Prerequisites.h"
#include "scene/ListenerComponent.h"

namespace sfmx
{

namespace script
{

void
registerListenerComponent(sol::state_view lua) {
  lua.new_usertype<ListenerComponent>("ListenerComponent",
    sol::no_constructor,
    sol::base_classes, sol::bases<Component>(),

    "typeId", sol::var(componentTypeId<ListenerComponent>()),

    "setAutoUpdate", &ListenerComponent::setAutoUpdate,
    "isAutoUpdate", &ListenerComponent::isAutoUpdate,

    "setPosition", &ListenerComponent::setPosition,
    "getPosition", &ListenerComponent::getPosition,
    "setDirection", &ListenerComponent::setDirection,
    "getDirection", &ListenerComponent::getDirection,
    "setUpVector", &ListenerComponent::setUpVector,
    "getUpVector", &ListenerComponent::getUpVector,
    "setVelocity", &ListenerComponent::setVelocity,
    "getVelocity", &ListenerComponent::getVelocity,
    "setGlobalVolume", &ListenerComponent::setGlobalVolume,
    "getGlobalVolume", &ListenerComponent::getGlobalVolume
  );
}

}  // namespace script

}  // namespace sfmx