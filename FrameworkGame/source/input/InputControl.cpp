#include "input/InputControl.h"

#include <algorithm>
#include <cstdlib>
#include <string>

#include "input/InputSystem.h"

namespace sfmx
{

namespace {

// Split "a/b/c" into its '/'-separated segments.
Vector<String>
splitPath(StringView path) {
  Vector<String> parts;
  size_t start = 0;
  while (start <= path.size()) {
    const size_t slash = path.find('/', start);
    if (StringView::npos == slash) {
      parts.emplace_back(path.substr(start));
      break;
    }
    parts.emplace_back(path.substr(start, slash - start));
    start = slash + 1;
  }
  return parts;
}

} // namespace

float
InputControl::read(const InputSystem& system) const {
  return system.sampleControl(*this);
}

String
InputControl::toPath() const {
  switch (m_device) {
    case DeviceType::kKeyboard:
      return "Keyboard/" + toString(static_cast<Key>(m_code));
    case DeviceType::kMouse:
      return "Mouse/" + toString(static_cast<MouseButton>(m_code));
    case DeviceType::kGamepad: {
      const int index = m_gamepadIndex < 0 ? 0 : m_gamepadIndex;
      const String prefix = "Gamepad" + std::to_string(index);
      if (m_isAxis) {
        return prefix + "/Axis/" + toString(static_cast<Axis>(m_code));
      }
      return prefix + "/Button/" + toString(static_cast<GamepadButton>(m_code));
    }
    default:
      return "Keyboard/Unknown";
  }
}

Optional<InputControl>
InputControl::tryFromPath(StringView path) {
  const Vector<String> parts = splitPath(path);
  if (parts.empty()) {
    return std::nullopt;
  }

  const String& device = parts[0];

  if ("Keyboard" == device) {
    if (parts.size() != 2) {
      return std::nullopt;
    }
    const Key key = keyFromString(parts[1]);
    // keyFromString falls back to kUnknown, which toString renders as "Unknown".
    // The round-trip check also rejects a non-canonical spelling.
    if (Key::kUnknown == key || toString(key) != parts[1]) {
      return std::nullopt;
    }
    InputControl control;
    control.m_device = DeviceType::kKeyboard;
    control.m_code = static_cast<int>(key);
    return control;
  }

  if ("Mouse" == device) {
    if (parts.size() != 2) {
      return std::nullopt;
    }
    const MouseButton button = mouseButtonFromString(parts[1]);
    // mouseButtonFromString falls back to kLeft, so validity is checked by
    // re-rendering the parsed value and comparing it with the input.
    if (toString(button) != parts[1]) {
      return std::nullopt;
    }
    InputControl control;
    control.m_device = DeviceType::kMouse;
    control.m_code = static_cast<int>(button);
    return control;
  }

  if (device.rfind("Gamepad", 0) == 0) {
    if (parts.size() != 3) {
      return std::nullopt;
    }

    // "Gamepad" alone, a non-numeric suffix, or an index outside the tracked
    // range is rejected rather than silently clamped to pad 0.
    const StringView indexText(device.data() + 7, device.size() - 7);
    if (indexText.empty()) {
      return std::nullopt;
    }
    for (const char c : indexText) {
      if (c < '0' || c > '9') {
        return std::nullopt;
      }
    }
    const int index = std::atoi(device.c_str() + 7);
    if (index < 0 || index >= kMaxGamepads) {
      return std::nullopt;
    }

    InputControl control;
    control.m_device = DeviceType::kGamepad;
    control.m_gamepadIndex = index;

    if ("Axis" == parts[1]) {
      const Axis axis = axisFromString(parts[2]);
      if (Axis::kCount == axis || toString(axis) != parts[2]) {
        return std::nullopt;
      }
      control.m_isAxis = true;
      control.m_code = static_cast<int>(axis);
    }
    else if ("Button" == parts[1]) {
      const GamepadButton button = gamepadButtonFromString(parts[2]);
      if (GamepadButton::kCount == button || toString(button) != parts[2]) {
        return std::nullopt;
      }
      control.m_isAxis = false;
      control.m_code = static_cast<int>(button);
    }
    else {
      return std::nullopt;
    }
    return control;
  }

  return std::nullopt;
}

InputControl
InputControl::fromPath(StringView path) {
  return tryFromPath(path).value_or(InputControl{});
}

InputValue
CompositeBinding::evaluate(const InputSystem& system) const {
  InputValue out;
  out.m_type = (CompositeType::kAxis1D == m_type) ? ActionValueType::kAxis1D
                                                  : ActionValueType::kAxis2D;
  float x = 0.f;
  float y = 0.f;

  for (const BindingPart& part : m_parts) {
    const float raw = part.m_control.read(system);
    const Vector2f processed =
      applyProcessors(part.m_processors, Vector2f{raw, 0.f}, out.m_type);
    const float value = processed.x;

    switch (part.m_role) {
      case CompositeRole::kPositiveX: x += value; break;
      case CompositeRole::kNegativeX: x -= value; break;
      case CompositeRole::kPositiveY: y += value; break;
      case CompositeRole::kNegativeY: y -= value; break;
      case CompositeRole::kAxisX:     x += value; break;
      case CompositeRole::kAxisY:     y += value; break;
      default: break;
    }
  }

  out.m_value.x = std::clamp(x, -1.f, 1.f);
  out.m_value.y = std::clamp(y, -1.f, 1.f);
  return out;
}

} // namespace sfmx
