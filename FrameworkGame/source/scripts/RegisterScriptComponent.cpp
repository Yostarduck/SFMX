#include "scripts/RegisterScriptComponent.h"

#include "core/platform/Prerequisites.h"
#include "scene/ScriptComponent.h"

namespace sfmx
{

namespace script
{

void
registerScriptComponent(sol::state_view lua) {
  lua.new_usertype<ScriptComponent>("ScriptComponent",
    sol::no_constructor,
    sol::base_classes, sol::bases<Component>(),

    "typeId", sol::var(componentTypeId<ScriptComponent>()),

    "isInitialized", &ScriptComponent::isInitialized
  );
}

}  // namespace script

}  // namespace sfmx