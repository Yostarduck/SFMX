#include "scripts/RegisterKeyboard.h"

#include "core/platform/Prerequisites.h"
#include "input/Keyboard.h"
#include "input/InputTypes.h"

namespace sfmx
{

namespace script
{

void
registerKeyboard(sol::state_view lua) {
  lua.new_usertype<Keyboard>("Keyboard",
    sol::no_constructor,

    "isPressed",            &Keyboard::isPressed,
    "wasPressedThisFrame",  &Keyboard::wasPressedThisFrame,
    "wasReleasedThisFrame", &Keyboard::wasReleasedThisFrame);

  lua["Keyboard"] = std::ref(Keyboard::instance());
}

}  // namespace script

}  // namespace sfmx