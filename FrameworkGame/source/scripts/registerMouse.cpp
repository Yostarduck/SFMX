#include "core/platform/Prerequisites.h"
#include "scripts/registerMouse.h"

#include "input/Mouse.h"
#include "input/InputTypes.h"

namespace sfmx
{

namespace script
{

void
RegisterMouse(sol::state_view lua) {
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

}

}