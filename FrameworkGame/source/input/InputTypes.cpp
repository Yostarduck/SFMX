#include "input/InputTypes.h"

#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

namespace sfmx
{

namespace {

// Key names, indexed by Key value (0 .. Key::kCount - 1), in the exact order
// of the Key enum. kUnknown is handled separately.
const ansichar* const kKeyNames[] = {
  "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
  "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
  "Num0", "Num1", "Num2", "Num3", "Num4", "Num5", "Num6", "Num7", "Num8", "Num9",
  "Escape", "LControl", "LShift", "LAlt", "LSystem",
  "RControl", "RShift", "RAlt", "RSystem", "Menu",
  "LBracket", "RBracket", "Semicolon", "Comma", "Period", "Apostrophe",
  "Slash", "Backslash", "Grave", "Equal", "Hyphen",
  "Space", "Enter", "Backspace", "Tab", "PageUp", "PageDown", "End", "Home",
  "Insert", "Delete", "Add", "Subtract", "Multiply", "Divide",
  "Left", "Right", "Up", "Down",
  "Numpad0", "Numpad1", "Numpad2", "Numpad3", "Numpad4",
  "Numpad5", "Numpad6", "Numpad7", "Numpad8", "Numpad9",
  "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
  "F11", "F12", "F13", "F14", "F15", "Pause"
};

const ansichar* const kMouseButtonNames[] = {
  "Left", "Right", "Middle", "Extra1", "Extra2"
};

const ansichar* const kAxisNames[] = {
  "LeftX", "LeftY", "RightX", "RightY", "LeftTrigger", "RightTrigger",
  "PovX", "PovY"
};

const ansichar* const kGamepadButtonNames[] = {
  "South", "East", "West", "North", "L1", "R1",
  "Select", "Start", "LStick", "RStick"
};

const ansichar* const kDeviceNames[] = { "Keyboard", "Mouse", "Gamepad" };

// Linear name lookup over a names table; returns the matching index or -1.
int
indexOfName(const ansichar* const* names, int count, StringView name) {
  for (int i = 0; i < count; ++i) {
    if (name == names[i]) {
      return i;
    }
  }
  return -1;
}

} // namespace

/************************************************************************/
/* SFML boundary conversions                                            */
/************************************************************************/

sf::Keyboard::Key
toSfml(Key key) {
  if (static_cast<int32>(key) < 0 || key >= Key::kCount) {
    return sf::Keyboard::Key::Unknown;
  }
  return static_cast<sf::Keyboard::Key>(static_cast<int>(key));
}

Key
keyFromSfml(sf::Keyboard::Key key) {
  const int value = static_cast<int>(key);
  if (value < 0 || value >= static_cast<int32>(Key::kCount)) {
    return Key::kUnknown;
  }
  return static_cast<Key>(value);
}

sf::Mouse::Button
toSfml(MouseButton button) {
  return static_cast<sf::Mouse::Button>(static_cast<int>(button));
}

MouseButton
mouseButtonFromSfml(sf::Mouse::Button button) {
  const int value = static_cast<int>(button);
  if (value < 0 || value >= static_cast<int32>(MouseButton::kCount)) {
    return MouseButton::kLeft;
  }
  return static_cast<MouseButton>(value);
}

sf::Joystick::Axis
toSfml(Axis axis) {
  // Best-effort semantic-to-physical mapping; distinct physical axis per
  // semantic axis. Trigger / right-stick layout is controller dependent.
  switch (axis) {
    case Axis::kLeftX:        return sf::Joystick::Axis::X;
    case Axis::kLeftY:        return sf::Joystick::Axis::Y;
    case Axis::kRightX:       return sf::Joystick::Axis::U;
    case Axis::kRightY:       return sf::Joystick::Axis::V;
    case Axis::kLeftTrigger:  return sf::Joystick::Axis::Z;
    case Axis::kRightTrigger: return sf::Joystick::Axis::R;
    case Axis::kPovX:         return sf::Joystick::Axis::PovX;
    case Axis::kPovY:         return sf::Joystick::Axis::PovY;
    default:                  return sf::Joystick::Axis::X;
  }
}

int
toSfmlButton(GamepadButton button) {
  return static_cast<int>(button);
}

GamepadButton
gamepadButtonFromSfml(int button) {
  if (button < 0 || button >= static_cast<int32>(GamepadButton::kCount)) {
    return GamepadButton::kCount;
  }
  return static_cast<GamepadButton>(button);
}

/************************************************************************/
/* Stable string names for serialization                                */
/************************************************************************/

String
toString(Key key) {
  if (static_cast<int32>(key) < 0 || key >= Key::kCount) {
    return "Unknown";
  }
  return kKeyNames[static_cast<int>(key)];
}

Key
keyFromString(StringView name) {
  const int index = indexOfName(kKeyNames, static_cast<int32>(Key::kCount), name);
  return index < 0 ? Key::kUnknown : static_cast<Key>(index);
}

String
toString(MouseButton button) {
  if (static_cast<int32>(button) < 0 || button >= MouseButton::kCount) {
    return "Left";
  }
  return kMouseButtonNames[static_cast<int>(button)];
}

MouseButton
mouseButtonFromString(StringView name) {
  const int index = indexOfName(kMouseButtonNames,
                                static_cast<int32>(MouseButton::kCount), name);
  return index < 0 ? MouseButton::kLeft : static_cast<MouseButton>(index);
}

String
toString(Axis axis) {
  if (static_cast<int32>(axis) < 0 || axis >= Axis::kCount) {
    return "LeftX";
  }
  return kAxisNames[static_cast<int>(axis)];
}

Axis
axisFromString(StringView name) {
  const int index = indexOfName(kAxisNames, static_cast<int32>(Axis::kCount), name);
  return index < 0 ? Axis::kLeftX : static_cast<Axis>(index);
}

String
toString(GamepadButton button) {
  if (static_cast<int32>(button) < 0 || button >= GamepadButton::kCount) {
    return "South";
  }
  return kGamepadButtonNames[static_cast<int>(button)];
}

GamepadButton
gamepadButtonFromString(StringView name) {
  const int index = indexOfName(kGamepadButtonNames,
                                static_cast<int32>(GamepadButton::kCount), name);
  return index < 0 ? GamepadButton::kCount : static_cast<GamepadButton>(index);
}

String
toString(DeviceType device) {
  if (static_cast<int32>(device) < 0 || device >= DeviceType::kCount) {
    return "Keyboard";
  }
  return kDeviceNames[static_cast<int>(device)];
}

DeviceType
deviceTypeFromString(StringView name) {
  const int index = indexOfName(kDeviceNames,
                                static_cast<int32>(DeviceType::kCount), name);
  return index < 0 ? DeviceType::kKeyboard : static_cast<DeviceType>(index);
}

} // namespace sfmx