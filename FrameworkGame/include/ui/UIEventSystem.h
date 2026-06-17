/************************************************************************/
/**
 * @file UIEventSystem.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Central input manager and focus controller for the UI system.
 *
 * Combines the role of Unity's EventSystem: owns the focused widget,
 * manages navigation (arrow keys, gamepad), and reads Submit/Cancel
 * actions from a Mapping.  A single Module singleton serves the entire
 * application; the active canvas registers itself for navigation queries.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"
#include "utils/EventSystem.h"
#include "utils/Module.h"

namespace sfmx
{

class Mapping;
class UIWidget;
class UICanvasComponent;

/**
 * @brief Module singleton that owns UI focus and processes navigation input.
 *
 * Call @ref init once after creating the UI Mapping, then call @ref beginFrame
 * at the start of each frame (after InputSystem::beginFrame).  The active
 * canvas calls @ref handleNavigation once per frame from its onUpdate.
 */
class UIEventSystem : public Module<UIEventSystem>
{
 public:
  /** @brief Subscribe to Submit / Cancel actions from a Mapping. */
  void init(Mapping* uiMapping);

  /** @brief Reset edge flags for a new frame. */
  void beginFrame();

  // ── Submit / Cancel ─────────────────────────────────────────────────

  /** @brief Whether the "Submit" action started this frame (Enter / Space / Gamepad South) */
  NODISCARD bool wasSubmitted() const { return m_submit; }

  /** @brief Whether the "Cancel" action started this frame (Escape / Gamepad East) */
  NODISCARD bool wasCancelled() const { return m_cancel; }

  // ── Navigation ──────────────────────────────────────────────────────

  /** @brief Run directional navigation (arrows / gamepad) for the registered canvas. */
  void handleNavigation();

  /** @brief Set the currently focused widget (may be nullptr). */
  void setFocus(UIWidget* w);

  /** @brief Currently focused widget (or nullptr). */
  NODISCARD UIWidget* getFocusedWidget() const { return m_focusedWidget; }

  // Navigation config

  /** @brief Widget that receives focus when the UI first becomes active. */
  FORCEINLINE void
  setFirstSelected(UIWidget* w) { m_firstSelected = w; }
  NODISCARD FORCEINLINE UIWidget*
  getFirstSelected() const { return m_firstSelected; }

  /** @brief Whether directional navigation wraps around when no candidate is found. */
  FORCEINLINE void
  setNavigationWrap(bool v) { m_navigationWrap = v; }
  NODISCARD FORCEINLINE bool
  getNavigationWrap() const { return m_navigationWrap; }

  // Canvas registration

  /** @brief Register the active canvas so navigation can walk its scene tree. */
  void registerCanvas(UICanvasComponent* canvas) { m_registeredCanvas = canvas; }

 protected:
  void onStartUp() override {}
  void onShutDown() override;

 private:
  friend class Module<UIEventSystem>;
  UIEventSystem() = default;

  void focusDirectional(int dx, int dy);
  sf::Vector2f widgetCenter(const UIWidget& w) const;

  HEvent m_submitSub;
  HEvent m_cancelSub;

  bool m_submit = false;
  bool m_cancel = false;

  // Focus / navigation state
  UIWidget*          m_focusedWidget    = nullptr;
  UIWidget*          m_firstSelected    = nullptr;
  bool               m_hasEverFocused   = false;
  bool               m_navigationWrap   = true;
  UICanvasComponent* m_registeredCanvas = nullptr;
};

} // namespace sfmx
