#include "input/Gamepad.h"

#include <algorithm>
#include <cmath>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Joystick.hpp>

namespace sfmx
{

namespace {

// Map a raw axis position in [-100, 100] to a dead-zoned value in [-1, 1].
float
normalizeAxis(float rawPosition, float deadZone) {
  const float normalized = std::clamp(rawPosition / 100.f, -1.f, 1.f);
  const float magnitude = std::fabs(normalized);
  if (magnitude <= deadZone) {
    return 0.f;
  }
  const float scaled = (magnitude - deadZone) / (1.f - deadZone);
  return std::copysign(std::clamp(scaled, 0.f, 1.f), normalized);
}

} // namespace

float
GamepadDevice::getAxis(Axis axis) const {
  if (static_cast<int32>(axis) < 0 || axis >= Axis::kCount) {
    return 0.f;
  }
  return m_axes[static_cast<size_t>(axis)];
}

bool
GamepadDevice::isPressed(GamepadButton button) const {
  if (static_cast<int32>(button) < 0 || button >= GamepadButton::kCount) {
    return false;
  }
  return m_current.test(static_cast<size_t>(button));
}

bool
GamepadDevice::wasPressedThisFrame(GamepadButton button) const {
  if (static_cast<int32>(button) < 0 || button >= GamepadButton::kCount) {
    return false;
  }
  const size_t index = static_cast<size_t>(button);
  return m_current.test(index) && !m_previous.test(index);
}

bool
GamepadDevice::wasReleasedThisFrame(GamepadButton button) const {
  if (static_cast<int32>(button) < 0 || button >= GamepadButton::kCount) {
    return false;
  }
  const size_t index = static_cast<size_t>(button);
  return !m_current.test(index) && m_previous.test(index);
}

void
GamepadDevice::beginFrame() {
  m_previous = m_current;
}

void
GamepadDevice::poll(int index) {
  const unsigned int joystick = static_cast<unsigned int>(index);
  m_connected = sf::Joystick::isConnected(joystick);
  if (!m_connected) {
    m_current.reset();
    m_axes.fill(0.f);
    return;
  }

  for (int button = 0; button < static_cast<int32>(GamepadButton::kCount); ++button) {
    const bool down = sf::Joystick::isButtonPressed(
      joystick, static_cast<unsigned int>(toSfmlButton(static_cast<GamepadButton>(button))));
    m_current.set(static_cast<size_t>(button), down);
  }

  for (int axis = 0; axis < static_cast<int32>(Axis::kCount); ++axis) {
    const sf::Joystick::Axis sfAxis = toSfml(static_cast<Axis>(axis));
    const float raw = sf::Joystick::getAxisPosition(joystick, sfAxis);
    m_axes[static_cast<size_t>(axis)] = normalizeAxis(raw, m_deadZone);
  }
}

GamepadDevice&
Gamepad::get(int index) {
  const int clamped = std::clamp(index, 0, kMaxGamepads - 1);
  return m_pads[static_cast<size_t>(clamped)];
}

int
Gamepad::getConnectedCount() const {
  int count = 0;
  for (const GamepadDevice& pad : m_pads) {
    if (pad.isConnected()) {
      ++count;
    }
  }
  return count;
}

bool
Gamepad::isConnected(int index) const {
  if (index < 0 || index >= kMaxGamepads) {
    return false;
  }
  return m_pads[static_cast<size_t>(index)].isConnected();
}

void
Gamepad::onEvent(const sf::Event& event) {
  // Connection edges are reflected by poll() reading sf::Joystick::isConnected;
  // the events are accepted here so the dispatcher has a home for them.
  (void)event;
}

void
Gamepad::beginFrame() {
  for (GamepadDevice& pad : m_pads) {
    pad.beginFrame();
  }
}

void
Gamepad::poll() {
  for (int index = 0; index < kMaxGamepads; ++index) {
    m_pads[static_cast<size_t>(index)].poll(index);
  }
}

} // namespace sfmx