#pragma once

#include "core/platform/Prerequisites.h"
#include "input/InputTypes.h"
#include "utils/Module.h"

namespace sf
{
class Event;
class WindowBase;
} // namespace sf

namespace sfmx
{

/**
 * @brief Cached mouse state: button edges, window-relative position, and the
 *        per-frame wheel delta.
 *
 * A @ref Module singleton driven by @ref InputSystem. Buttons use the same
 * current/previous snapshot scheme as @ref Keyboard; position is polled in
 * @ref pollPosition and the wheel delta is accumulated from events and reset
 * each frame in @ref beginFrame.
 */
class Mouse : public Module<Mouse>
{
 public:
  /** @brief True while @p button is held this frame. */
  NODISCARD bool
  isPressed(MouseButton::E button) const;

  /** @brief True only on the frame @p button transitions from up to down. */
  NODISCARD bool
  wasPressedThisFrame(MouseButton::E button) const;

  /** @brief True only on the frame @p button transitions from down to up. */
  NODISCARD bool
  wasReleasedThisFrame(MouseButton::E button) const;

  /** @brief Window-relative cursor position, as of the last @ref pollPosition. */
  NODISCARD Vector2i
  getPosition() const;

  /** @brief Wheel scroll accumulated this frame (reset each @ref beginFrame). */
  NODISCARD float
  getWheelDelta() const;

  /** @brief (Internal) Apply one SFML event. */
  void
  onEvent(const sf::Event& event);

  /** @brief (Internal) Refresh the cached position from @p window. */
  void
  pollPosition(const sf::WindowBase& window);

  /** @brief (Internal) Snapshot buttons and clear the wheel delta. */
  void
  beginFrame();

 private:
  friend class Module<Mouse>;

  Mouse() = default;

  BitSet<MouseButton::kCount> m_current;
  BitSet<MouseButton::kCount> m_previous;
  Vector2i m_position;
  float m_wheelDelta = 0.f;
};

} // namespace sfmx
