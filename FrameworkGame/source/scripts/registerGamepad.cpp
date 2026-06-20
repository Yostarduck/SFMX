#include "core/platform/Prerequisites.h"
#include "scripts/registerGamepad.h"

#include "input/Gamepad.h"
#include "input/InputTypes.h"

namespace sfmx
{

namespace script
{

void
RegisterGamepad(sol::state_view lua) {
  lua.new_usertype<GamepadDevice>("GamepadDevice",
    sol::no_constructor,
    
    "getAxis",              &GamepadDevice::getAxis,

    "isPressed",            &GamepadDevice::isPressed,
    "wasPressedThisFrame",  &GamepadDevice::wasPressedThisFrame,
    "wasReleasedThisFrame", &GamepadDevice::wasReleasedThisFrame,

    "isConnected",          &GamepadDevice::isConnected,
    
    "getDeadZone",          &GamepadDevice::getDeadZone,
    "setDeadZone",          &GamepadDevice::setDeadZone
  );

  lua.new_usertype<Gamepad>("Gamepad",
    sol::no_constructor,

    "get",                &Gamepad::get,
    "getConnectedCount",  &Gamepad::getConnectedCount,
    "isConnected",        &Gamepad::isConnected
  );

  lua["Gamepad"] = std::ref(Gamepad::instance());
}

}

}