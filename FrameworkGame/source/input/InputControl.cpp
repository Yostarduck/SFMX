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

InputControl
InputControl::fromPath(StringView path) {
  InputControl control;
  const Vector<String> parts = splitPath(path);
  if (parts.empty()) {
    return control;
  }

  const String& device = parts[0];
  if ("Keyboard" == device && parts.size() >= 2) {
    control.m_device = DeviceType::kKeyboard;
    control.m_code = static_cast<int>(keyFromString(parts[1]));
  }
  else if ("Mouse" == device && parts.size() >= 2) {
    control.m_device = DeviceType::kMouse;
    control.m_code = static_cast<int>(mouseButtonFromString(parts[1]));
  }
  else if (device.rfind("Gamepad", 0) == 0 && parts.size() >= 3) {
    control.m_device = DeviceType::kGamepad;
    control.m_gamepadIndex = device.size() > 7 ? std::atoi(device.c_str() + 7) : 0;
    if ("Axis" == parts[1]) {
      control.m_isAxis = true;
      control.m_code = static_cast<int>(axisFromString(parts[2]));
    }
    else {
      control.m_isAxis = false;
      control.m_code = static_cast<int>(gamepadButtonFromString(parts[2]));
    }
  }
  return control;
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
