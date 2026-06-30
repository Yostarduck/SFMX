#pragma once

#include "core/platform/Prerequisites.h"
#include "input/Mapping.h"
#include "utils/Module.h"


namespace sf
{
class Event;
class WindowBase;
} // namespace sf

namespace sfmx
{

struct InputControl;
class Mapping;

/**
 * @brief Process-wide façade and owner of the input subsystem.
 *
 * Wires into the game loop at three points each frame:
 *   1. @ref beginFrame before polling — snapshots device state for edge detection.
 *   2. @ref onEvent per SFML event — routes to the owning device.
 *   3. @ref update once per frame — polls continuous state (mouse, gamepads) and
 *      evaluates the active Mapping.
 *
 * Owns and drives the @ref Keyboard, @ref Mouse, and @ref Gamepad singletons;
 * game code reads those directly for direct-mode queries.
 */
class InputSystem : public Module<InputSystem>
{
 public:
  /** @brief Out-of-line so owned Mappings are destroyed where they are complete. */
  ~InputSystem() override;

  /** @brief Snapshot every device (previous = current) before event polling. */
  void
  beginFrame();

  /** @brief Route one SFML event to the device that owns it. */
  void
  onEvent(const sf::Event& event);

  /**
   * @brief Advance the frame: poll continuous device state, then evaluate the
   *        active mapping (firing action events).
   * @param deltaTime Seconds since the previous update.
   * @param window    Window used to resolve the mouse position.
   */
  void
  update(float deltaTime, const sf::WindowBase& window);

  /**
   * @brief Sample one control's current value: 1/0 for buttons, [-1,1] for axes.
   *        The single device-read path used by bindings.
   */
  NODISCARD float
  sampleControl(const InputControl& control) const;

  /** @brief Create and own a new (empty) mapping; returns a non-owning pointer. */
  Mapping*
  createMapping(StringView name);

  /** @brief Make @p mapping the one evaluated each frame (may be nullptr). */
  void
  setActiveMapping(Mapping* mapping) { m_activeMapping = mapping; }

  NODISCARD Mapping*
  getActiveMapping() const { return m_activeMapping; }

 protected:
  /** @brief Starts the device singletons. */
  void
  onStartUp() override;

  /** @brief Shuts the device singletons down (reverse order). */
  void
  onShutDown() override;

 private:
  friend class Module<InputSystem>;

  InputSystem() = default;

  Vector<UniquePtr<Mapping>> m_mappings;
  Mapping* m_activeMapping = nullptr;
};

} // namespace sfmx
