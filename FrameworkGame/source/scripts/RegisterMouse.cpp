#include "scripts/RegisterMouse.h"

#include "core/platform/Prerequisites.h"
#include "input/Mouse.h"
#include "input/InputTypes.h"

namespace sfmx
{

namespace script
{

void
registerMouse(sol::state_view lua) {
  lua.new_usertype<Mouse>("Mouse",
    sol::no_constructor,

    "isPressed",            &Mouse::isPressed,
    "wasPressedThisFrame",  &Mouse::wasPressedThisFrame,
    "wasReleasedThisFrame", &Mouse::wasReleasedThisFrame,
    "getPosition",          &Mouse::getPosition,
    "getWheelDelta",        &Mouse::getWheelDelta
  );

  lua["Mouse"] = std::ref(Mouse::instance());
}

}  // namespace script

}  // namespace sfmx