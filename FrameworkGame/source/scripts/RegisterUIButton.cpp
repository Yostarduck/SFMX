#include "scripts/RegisterUIButton.h"

#include "ui/UIButton.h"
#include "scene/ScriptComponent.h"

#include "core/platform/Prerequisites.h"
#include <SFML/System/Vector2.hpp>

namespace sfmx
{

namespace script
{

void
registerUIButton(sol::state_view lua) {
  lua.new_usertype<UIButton>("UIButton",
    sol::no_constructor,
    sol::base_classes, sol::bases<UIWidget, Component>(),
    
    // Stamps the concrete type id onto the usertype so Lua can drive
    // node:addComponent(CameraComponent, ...) etc. (see registerComponentAccess).
    "typeId", sol::var(componentTypeId<UIButton>())
  );
}

}  // namespace script

}  // namespace sfmx