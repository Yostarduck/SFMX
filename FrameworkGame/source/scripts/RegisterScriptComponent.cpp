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

    "isInitialized", &ScriptComponent::isInitialized,

    // Returns the script's table (its hooks plus any fields the script publishes)
    // so another script can share state, e.g. other:instance().speed = 200.
    // nil while the script is unbound or disabled.
    "instance", [](sol::this_state state, ScriptComponent& self) -> sol::object {
      const sol::table& table = self.getInstanceTable();
      if (!table.valid()) {
        return sol::make_object(state, sol::lua_nil);
      }
      return sol::make_object(state, table);
    }
  );
}

}  // namespace script

}  // namespace sfmx