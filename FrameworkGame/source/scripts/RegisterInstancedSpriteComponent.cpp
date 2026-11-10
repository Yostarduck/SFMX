#include "scripts/RegisterInstancedSpriteComponent.h"

#include "core/platform/Prerequisites.h"
#include "scene/InstancedSpriteComponent.h"

namespace sfmx {

namespace script {

void 
registerInstancedSpriteComponent(sol::state_view lua) {
  lua.new_usertype<InstancedSpriteComponent>("InstancedSpriteComponent",
    sol::no_constructor,
    sol::base_classes, 
    sol::bases<Component>(),

    "typeId", sol::var(componentTypeId<InstancedSpriteComponent>()),

    "setAtlas", &InstancedSpriteComponent::setAtlas,
    "setTexture", &InstancedSpriteComponent::setTexture,
    "setFrame", &InstancedSpriteComponent::setFrame,
    "setSize", &InstancedSpriteComponent::setSize,
    "setRotation", &InstancedSpriteComponent::setRotation,
    "setTint", &InstancedSpriteComponent::setTint,
    "getFrame", &InstancedSpriteComponent::getFrame,
    "getSize", &InstancedSpriteComponent::getSize,
    "getRotation", &InstancedSpriteComponent::getRotation,
    "getTint", &InstancedSpriteComponent::getTint
  );
}

} // namespace script

} // namespace sfmx