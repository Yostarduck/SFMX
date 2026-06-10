#pragma once

#include "core/platform/Prerequisites.h"
#include "input/InputTypes.h"
#include "utils/Module.h"

namespace sf { class Event; }

namespace sfmx
{

class Gamepad;

/**
 * @brief State of a single physical gamepad (one of @ref kMaxGamepads).
 *
 * Polled each frame from @c sf::Joystick by the owning @ref Gamepad. Axis values
 * are normalized to [-1, 1] and dead-zoned on read; buttons use the same
 * current/previous snapshot scheme as the other devices.
 */
class GamepadDevice
{
 public:
  GamepadDevice() = default;

  /** @brief Normalized, dead-zoned axis value in [-1, 1]. */
  NODISCARD float
  getAxis(Axis::E axis) const;

  /** @brief True while @p button is held this frame. */
  NODISCARD bool
  isPressed(GamepadButton::E button) const;

  /** @brief True only on the frame @p button transitions from up to down. */
  NODISCARD bool
  wasPressedThisFrame(GamepadButton::E button) const;

  /** @brief True only on the frame @p button transitions from down to up. */
  NODISCARD bool
  wasReleasedThisFrame(GamepadButton::E button) const;

  /** @brief Whether this pad was connected as of the last poll. */
  NODISCARD bool
  isConnected() const { return m_connected; }

  /** @brief Dead-zone applied to axes, as a fraction of full deflection. */
  NODISCARD float
  getDeadZone() const { return m_deadZone; }

  void
  setDeadZone(float deadZone) { m_deadZone = deadZone; }

 private:
  friend class Gamepad;

  /** @brief Snapshot button state current -> previous. */
  void
  beginFrame();

  /** @brief Refresh connection, buttons, and axes from joystick @p index. */
  void
  poll(int index);

  BitSet<GamepadButton::kCount> m_current;
  BitSet<GamepadButton::kCount> m_previous;
  Array<float, Axis::kCount> m_axes{};
  float m_deadZone = kDefaultDeadZone;
  bool m_connected = false;
};

/**
 * @brief Multiplexer over the connected gamepads.
 *
 * A @ref Module singleton driven by @ref InputSystem. Holds fixed storage for
 * @ref kMaxGamepads pads (no per-frame allocation) and forwards polling to each.
 */
class Gamepad : public Module<Gamepad>
{
 public:
  /** @brief Access pad @p index (0-based, clamped into range). */
  NODISCARD GamepadDevice&
  get(int index);

  /** @brief Number of currently connected pads. */
  NODISCARD int
  getConnectedCount() const;

  /** @brief Whether pad @p index is connected. */
  NODISCARD bool
  isConnected(int index) const;

  /** @brief (Internal) Apply one SFML event (connect / disconnect). */
  void
  onEvent(const sf::Event& event);

  /** @brief (Internal) Snapshot every pad's button state. */
  void
  beginFrame();

  /** @brief (Internal) Poll every pad from the joystick backend. */
  void
  poll();

 private:
  friend class Module<Gamepad>;

  Gamepad() = default;

  Array<GamepadDevice, kMaxGamepads> m_pads;
};

} // namespace sfmx
