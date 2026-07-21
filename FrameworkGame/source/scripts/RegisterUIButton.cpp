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
    sol::call_constructor,
    sol::constructors<UIButton()>(),

    "setEnabled", &UIButton::setEnabled,

    "onSubmit", [](const ScriptComponent& caller, const String& fnName) {
    }
  );
}

}  // namespace script

}  // namespace sfmx