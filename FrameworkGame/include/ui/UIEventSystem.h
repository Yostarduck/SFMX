/************************************************************************/
/**
 * @file UIEventSystem.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Central input manager for the UI system.
 *
 * Replaces the per-canvas UIInputAdapter with a single Module singleton
 * that reads input from a Mapping (for submit/cancel/tab) and from
 * Mouse/Keyboard directly (for pointer/navigation).  This decouples UI
 * event handling from the device while keeping the pointer path simple.
 */
/************************************************************************/
#pragma once

#include "core/platform/Prerequisites.h"
#include "utils/EventSystem.h"
#include "utils/Module.h"

namespace sfmx
{

class Mapping;

/**
 * @brief Module singleton that provides UI-relevant input state each frame.
 *
 * Call @ref init once after creating the UI Mapping, then call @ref beginFrame
 * at the start of each frame (after InputSystem::beginFrame) and read the
 * getters from UICanvasComponent::onUpdate.
 */
class UIEventSystem : public Module<UIEventSystem>
{
 public:
  /** @brief Subscribe to actions from a Mapping (returns the action map). */
  void init(Mapping* uiMapping);

  /** @brief Reset edge flags for a new frame. */
  void beginFrame();

  // ── Submit / Cancel ─────────────────────────────────────────────────

  /** @brief Whether the "Submit" action started this frame (Enter / Space / Gamepad South) */
  NODISCARD bool wasSubmitted() const { return m_submit; }

  /** @brief Whether the "Cancel" action started this frame (Escape / Gamepad East) */
  NODISCARD bool wasCancelled() const { return m_cancel; }

  // ── Tab ─────────────────────────────────────────────────────────────

  /** @brief Whether the "Tab" action started this frame */
  NODISCARD bool wasTabPressed() const { return m_tab; }

  /** @brief Whether Shift is held (for Tab cycling direction) */
  NODISCARD bool isShiftHeld() const { return m_shift; }

 protected:
  void onStartUp() override {}
  void onShutDown() override;

 private:
  friend class Module<UIEventSystem>;
  UIEventSystem() = default;

  HEvent m_submitSub;
  HEvent m_cancelSub;
  HEvent m_tabSub;

  bool m_submit = false;
  bool m_cancel = false;
  bool m_tab    = false;
  bool m_shift  = false;
};

} // namespace sfmx
