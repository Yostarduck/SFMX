#pragma once

#include "core/platform/Prerequisites.h"
#include "input/InputTypes.h"

namespace sfmx
{

/** @brief State-machine phase of an action this frame (Unity/Unreal parity). */
enum class ActionPhase : int32 { kDisabled, kWaiting, kStarted, kPerformed, kCanceled };

/** @brief Timing model that decides when an action's phases fire. */
enum class InteractionType : int32 { kPassThrough, kPress, kHold, kTap, kMultiTap };

/**
 * @brief Decides, from the actuation flag and elapsed time, which phase an
 *        action is in this frame.
 *
 * A tagged value with a little runtime scratch. @ref step advances the machine
 * each frame and returns the resulting phase; the owning @ref InputAction turns
 * phase transitions into started / performed / canceled events.
 */
struct Interaction
{
  InteractionType m_type = InteractionType::kPassThrough;
  float m_duration = kDefaultHoldTime;       //!< Hold threshold / tap max time.
  float m_tapSpacing = kDefaultTapSpacing;   //!< Max gap between multi-taps.
  int m_tapCount = 2;                        //!< Taps required (kMultiTap).

  // Runtime scratch (not serialized).
  float m_elapsed = 0.f;
  int m_progress = 0;
  bool m_wasActuated = false;
  bool m_flag = false;                       //!< holdFired / tapFailed.

  /**
   * @brief Advance one frame.
   * @param actuated  Whether the action's value is non-zero this frame.
   * @param deltaTime Seconds since the previous step.
   * @return The phase for this frame.
   */
  ActionPhase
  step(bool actuated, float deltaTime);
};

} // namespace sfmx
