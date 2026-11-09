#include "scripts/RegisterInstancedSpriteComponent.h"

#include "scene/InstancedSpriteComponent.h"

namespace sfmx {

namespace script {

  void registerInstancedSpriteComponent(sol::state_view lua) {
    lua.new_usertype<InstancedSpriteComponent>("InstancedSpriteComponent",
      sol::base_classes, sol::bases<Component>(),

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