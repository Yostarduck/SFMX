#pragma once

#include "core/platform/Prerequisites.h"
#include "input/InputTypes.h"
#include "utils/Module.h"

namespace sf { class Event; }

namespace sfmx
{

/**
 * @brief Cached keyboard state with exact per-frame edge detection.
 *
 * A @ref Module singleton so game code can read it directly
 * (@c Keyboard::instance().isPressed(Key::kSpace)), but it is driven by
 * @ref InputSystem, which feeds it events and advances its frame snapshot.
 * Down-state is kept for the current and previous frame and diffed for edges.
 */
class Keyboard : public Module<Keyboard>
{
 public:
  /** @brief True while @p key is held this frame. */
  NODISCARD bool
  isPressed(Key::E key) const;

  /** @brief True only on the frame @p key transitions from up to down. */
  NODISCARD bool
  wasPressedThisFrame(Key::E key) const;

  /** @brief True only on the frame @p key transitions from down to up. */
  NODISCARD bool
  wasReleasedThisFrame(Key::E key) const;

  /** @brief (Internal, driven by InputSystem) Apply one SFML event. */
  void
  onEvent(const sf::Event& event);

  /** @brief (Internal, driven by InputSystem) Snapshot current -> previous. */
  void
  beginFrame();

 private:
  friend class Module<Keyboard>;

  Keyboard() = default;

  BitSet<Key::kCount> m_current;
  BitSet<Key::kCount> m_previous;
};

} // namespace sfmx
