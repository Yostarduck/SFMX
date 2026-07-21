#include "scripts/RegisterUIButton.h"

#include "ui/UIButton.h"
#include "scene/ScriptComponent.h"

#include "core/platform/Prerequisites.h"

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
    "typeId", sol::var(componentTypeId<UIButton>()),

    "setEnabled", &UIButton::setEnabled,

    "onSubmit", [](const UIButton& caller,
                   ScriptComponent& target,
                   const String& fnName) {
                    
      Function<void()> cb = [&target, fnName]() {
        target.executeFunction(fnName);
      };
      
      target.registerEvent(caller.onSubmit(std::move(cb)));
    }
  );
}

}  // namespace script

}  // namespace sfmx