#include "scripts/registerInputTypes.h"

#include "core/platform/Prerequisites.h"
#include "input/InputTypes.h"

namespace sfmx
{

namespace script
{

void
registerInputTypes(sol::state_view lua) {
  lua.new_enum<DeviceType>("DeviceType", {
    { "Keyboard",  DeviceType::kKeyboard },
    { "Mouse",     DeviceType::kMouse },
    { "Gamepad",   DeviceType::kGamepad },
    { "Count",     DeviceType::kCount }
  });

  lua.new_enum<Key>("Key", {
    { "Unknown", Key::kUnknown },

    { "A", Key::kA }, { "B", Key::kB }, { "C", Key::kC }, { "D", Key::kD },
    { "E", Key::kE }, { "F", Key::kF }, { "G", Key::kG }, { "H", Key::kH },
    { "I", Key::kI }, { "J", Key::kJ }, { "K", Key::kK }, { "L", Key::kL },
    { "M", Key::kM }, { "N", Key::kN }, { "O", Key::kO }, { "P", Key::kP },
    { "Q", Key::kQ }, { "R", Key::kR }, { "S", Key::kS }, { "T", Key::kT },
    { "U", Key::kU }, { "V", Key::kV }, { "W", Key::kW }, { "X", Key::kX },
    { "Y", Key::kY }, { "Z", Key::kZ },

    { "Num0", Key::kNum0 }, { "Num1", Key::kNum1 }, { "Num2", Key::kNum2 },
    { "Num3", Key::kNum3 }, { "Num4", Key::kNum4 }, { "Num5", Key::kNum5 },
    { "Num6", Key::kNum6 }, { "Num7", Key::kNum7 }, { "Num8", Key::kNum8 },
    { "Num9", Key::kNum9 },

    { "Escape",   Key::kEscape },

    { "LControl", Key::kLControl }, { "LShift",   Key::kLShift },
    { "LAlt",     Key::kLAlt },     { "LSystem",  Key::kLSystem },

    { "RControl", Key::kRControl }, { "RShift",   Key::kRShift },
    { "RAlt",     Key::kRAlt },     { "RSystem",  Key::kRSystem },

    { "Menu",     Key::kMenu },

    { "LBracket",   Key::kLBracket },  { "RBracket",    Key::kRBracket },
    { "Semicolon",  Key::kSemicolon }, { "Comma",       Key::kComma },
    { "Period",     Key::kPeriod },    { "Apostrophe",  Key::kApostrophe },
    { "Slash",      Key::kSlash },     { "Backslash",   Key::kBackslash },
    { "Grave",      Key::kGrave },     { "Equal",       Key::kEqual },
    { "Hyphen",     Key::kHyphen },

    { "Space",      Key::kSpace },      { "Enter",    Key::kEnter },
    { "Backspace",  Key::kBackspace },  { "Tab",      Key::kTab },
    { "PageUp",     Key::kPageUp },     { "PageDown", Key::kPageDown },
    { "End",        Key::kEnd },        { "Home",     Key::kHome },
    { "Insert",     Key::kInsert },     { "Delete",   Key::kDelete },
    { "Add",        Key::kAdd },        { "Subtract", Key::kSubtract },
    { "Multiply",   Key::kMultiply },   { "Divide",   Key::kDivide },

    { "Left",   Key::kLeft },
    { "Right",  Key::kRight },
    { "Up",     Key::kUp },
    { "Down",   Key::kDown },

    { "Numpad0", Key::kNumpad0 }, { "Numpad1", Key::kNumpad1 },
    { "Numpad2", Key::kNumpad2 }, { "Numpad3", Key::kNumpad3 },
    { "Numpad4", Key::kNumpad4 }, { "Numpad5", Key::kNumpad5 },
    { "Numpad6", Key::kNumpad6 }, { "Numpad7", Key::kNumpad7 },
    { "Numpad8", Key::kNumpad8 }, { "Numpad9", Key::kNumpad9 },

    { "F1",   Key::kF1 },   { "F2",   Key::kF2 },   { "F3",   Key::kF3 },
    { "F4",   Key::kF4 },   { "F5",   Key::kF5 },   { "F6",   Key::kF6 },
    { "F7",   Key::kF7 },   { "F8",   Key::kF8 },   { "F9",   Key::kF9 },
    { "F10",  Key::kF10 },  { "F11",  Key::kF11 },  { "F12",  Key::kF12 },
    { "F13",  Key::kF13 },  { "F14",  Key::kF14 },  { "F15",  Key::kF15 },

    { "Pause", Key::kPause },

    { "Count", Key::kCount }
  });

  lua.new_enum<MouseButton>("MouseButton", {
    { "Left",   MouseButton::kLeft },
    { "Right",  MouseButton::kRight },
    { "Middle", MouseButton::kMiddle },
    { "Extra1", MouseButton::kExtra1 },
    { "Extra2", MouseButton::kExtra2 },
    { "Count",  MouseButton::kCount }
  });

  lua.new_enum<Axis>("Axis", {
    { "LeftX",         Axis::kLeftX },
    { "LeftY",         Axis::kLeftY },

    { "RightX",        Axis::kRightX },
    { "RightY",        Axis::kRightY },

    { "LeftTrigger",   Axis::kLeftTrigger },
    { "RightTrigger",  Axis::kRightTrigger },

    { "PovX",          Axis::kPovX },
    { "PovY",          Axis::kPovY },

    { "Count",         Axis::kCount }
  });

  lua.new_enum<GamepadButton>("GamepadButton", {
    { "South",  GamepadButton::kSouth },
    { "East",   GamepadButton::kEast },
    { "West",   GamepadButton::kWest },
    { "North",  GamepadButton::kNorth },

    { "L1",     GamepadButton::kL1 },
    { "R1",     GamepadButton::kR1 },

    { "Select", GamepadButton::kSelect },
    { "Start",  GamepadButton::kStart },

    { "LStick", GamepadButton::kLStick },
    { "RStick", GamepadButton::kRStick },

    { "Count",  GamepadButton::kCount }
  });

  lua.set_function("toString", sol::overload(
    static_cast<String (*)(Key)>(&toString),
    static_cast<String (*)(MouseButton)>(&toString),
    static_cast<String (*)(Axis)>(&toString),
    static_cast<String (*)(GamepadButton)>(&toString),
    static_cast<String (*)(DeviceType)>(&toString)
  ));

  lua.set_function("keyFromString",           &keyFromString);
  lua.set_function("mouseButtonFromString",   &mouseButtonFromString);
  lua.set_function("axisFromString",          &axisFromString);
  lua.set_function("gamepadButtonFromString", &gamepadButtonFromString);
  lua.set_function("deviceTypeFromString",    &deviceTypeFromString);
}

}  // namespace script

}  // namespace sfmx