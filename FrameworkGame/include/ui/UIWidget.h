/************************************************************************/
/**
 * @file UIWidget.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Base class (mixin) for all interactive UI widgets.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"
#include "utils/EventSystem.h"

namespace sfmx
{

class SceneNode;

/**
 * @brief  Helper to express widget position and size relative to a parent rect.
 *
 * Each value is a fraction [0, 1] of the parent's size.  When left == right
 * the widget keeps its fixed width; when top == bottom it keeps its fixed
 * height.  Otherwise the anchor stretch forces the widget to fill that axis.
 */
// ── Anchor helpers ──────────────────────────────────────────────────────────

struct Anchors {
  float left   = 0.f;
  float top    = 0.f;
  float right  = 1.f;
  float bottom = 1.f;

  static Anchors fill()           { return {0.f, 0.f, 1.f, 1.f}; }
  static Anchors center()         { return {0.5f, 0.5f, 0.5f, 0.5f}; }
  static Anchors topLeft()        { return {0.f, 0.f, 0.f, 0.f}; }
  static Anchors topRight()       { return {1.f, 0.f, 1.f, 0.f}; }
  static Anchors bottomLeft()     { return {0.f, 1.f, 0.f, 1.f}; }
  static Anchors bottomRight()    { return {1.f, 1.f, 1.f, 1.f}; }
  static Anchors stretchH(float l, float r) { return {l, 0.f, r, 1.f}; }
  static Anchors stretchV(float t, float b) { return {0.f, t, 1.f, b}; }
};

enum class NavigationMode : uint8 {
  Automatic,  // geometric nearest neighbor (default)
  Explicit,   // manually assigned up/down/left/right targets
  Horizontal, // geometric, constrained to x-axis
  Vertical,   // geometric, constrained to y-axis
  None        // keyboard/gamepad navigation disabled
};

/**
 * @brief  Mixin that adds interactivity, layout, and events to a Component.
 *
 * Every UI widget inherits from both ComponentT<...> and UIWidget.
 * Provides size, anchors, pivot, hover/press/focus state, events, and
 * navigation mode support used by UICanvasComponent.
 */
class UIWidget {
 public:
  /** @brief Construct a widget attached to @p owner */
  explicit UIWidget(SceneNode* owner);

  // Size

  /** @brief Set the widget size in layout-space pixels */
  FORCEINLINE void
  setSize(sf::Vector2f s) { m_size = s; }
  /** @brief Current widget size */
  NODISCARD FORCEINLINE sf::Vector2f
  getSize() const         { return m_size; }

  // Anchors / pivot / offset

  /** @brief Set anchor fractions relative to the parent rectangle */
  FORCEINLINE void
  setAnchors(const Anchors& a) { m_anchors = a; }
  /** @brief Current anchor configuration */
  NODISCARD FORCEINLINE const Anchors& getAnchors() const { return m_anchors; }
  /** @brief Pivot fraction within the widget (0,0 = top-left, 0.5 = center) */
  FORCEINLINE void
  setPivot(sf::Vector2f p)     { m_pivot = p; }
  /** @brief Current pivot */
  NODISCARD FORCEINLINE sf::Vector2f getPivot() const { return m_pivot; }
  /** @brief Extra position offset applied after anchors and pivot */
  FORCEINLINE void
  setOffset(sf::Vector2f o)    { m_offset = o; }
  /** @brief Current offset */
  NODISCARD FORCEINLINE sf::Vector2f getOffset() const { return m_offset; }

  // Interactivity

  /** @brief Whether the widget responds to pointer input */
  FORCEINLINE void
  setInteractable(bool v) { m_interactable = v; }
  /** @brief Whether the widget responds to pointer input */
  NODISCARD FORCEINLINE bool isInteractable() const { return m_interactable; }
  /** @brief Whether the pointer is currently over this widget */
  NODISCARD FORCEINLINE bool isHovered() const    { return m_hovered; }
  /** @brief Whether the pointer button is currently held on this widget */
  NODISCARD FORCEINLINE bool isPressed() const    { return m_pressed; }
  /** @brief Whether this widget blocks input to widgets behind it */
  NODISCARD FORCEINLINE bool isConsumingInput() const { return m_consumesInput; }
  /** @brief Set whether this widget blocks input to widgets behind it */
  FORCEINLINE void setConsumesInput(bool v) { m_consumesInput = v; }

  // State setters (fire events)

  /** @brief Set hover state, fires onHoverEnter / onHoverExit */
  void setHovered(bool v);
  /** @brief Set press state, fires onPress / onRelease and onClick on release-while-hovered */
  void setPressed(bool v);
  /** @brief Set focus state, fires onFocusGained / onFocusLost */
  void setFocused(bool v);

  // Focus

  /** @brief Whether this widget currently has focus */
  NODISCARD FORCEINLINE bool isFocused() const   { return m_focused; }
  /** @brief Whether this widget can receive focus */
  NODISCARD FORCEINLINE bool isFocusable() const { return m_focusable; }
  /** @brief Set whether this widget can receive focus */
  FORCEINLINE void setFocusable(bool v) { m_focusable = v; }

  // Navigation mode (Unity-style)

  /** @brief Set the directional navigation mode for this widget */
  FORCEINLINE void setNavigationMode(NavigationMode m) { m_navMode = m; }
  /** @brief Current navigation mode */
  NODISCARD FORCEINLINE NavigationMode getNavigationMode() const { return m_navMode; }

  /** @brief Assign the explicit neighbour above */
  FORCEINLINE void setNavUp(UIWidget* w)    { m_navUp = w; }
  /** @brief Assign the explicit neighbour below */
  FORCEINLINE void setNavDown(UIWidget* w)  { m_navDown = w; }
  /** @brief Assign the explicit neighbour to the left */
  FORCEINLINE void setNavLeft(UIWidget* w)  { m_navLeft = w; }
  /** @brief Assign the explicit neighbour to the right */
  FORCEINLINE void setNavRight(UIWidget* w) { m_navRight = w; }
  /** @brief The explicit neighbour above (or nullptr) */
  NODISCARD FORCEINLINE UIWidget* getNavUp() const    { return m_navUp; }
  /** @brief The explicit neighbour below (or nullptr) */
  NODISCARD FORCEINLINE UIWidget* getNavDown() const  { return m_navDown; }
  /** @brief The explicit neighbour to the left (or nullptr) */
  NODISCARD FORCEINLINE UIWidget* getNavLeft() const  { return m_navLeft; }
  /** @brief The explicit neighbour to the right (or nullptr) */
  NODISCARD FORCEINLINE UIWidget* getNavRight() const { return m_navRight; }

  // Text input

  /** @brief Called with the frame's text-input buffer (routed by the canvas) */
  virtual void onTextInput(const Vector<char32_t>&) {}

  // Layout

  /** @brief Whether this widget arranges its children (e.g. UIHBox/UIVBox) */
  NODISCARD FORCEINLINE virtual bool isLayoutContainer() const { return false; }

  /** @brief The widget rectangle in canvas-space pixels */
  NODISCARD sf::FloatRect getWorldRect() const;

  /** @brief Recalculate position from anchor/pivot/offset within @p parentRect */
  void resolveLayout(sf::FloatRect parentRect);

  // Events

  /** @brief Fires on mouse click (press + release while hovered) */
  NODISCARD FORCEINLINE Event<void()>& onClick()       { return m_onClick; }
  /** @brief Fires when the pointer enters the widget */
  NODISCARD FORCEINLINE Event<void()>& onHoverEnter()  { return m_onHoverEnter; }
  /** @brief Fires when the pointer leaves the widget */
  NODISCARD FORCEINLINE Event<void()>& onHoverExit()   { return m_onHoverExit; }
  /** @brief Fires when the pointer button is pressed on the widget */
  NODISCARD FORCEINLINE Event<void()>& onPress()       { return m_onPress; }
  /** @brief Fires when the pointer button is released on the widget */
  NODISCARD FORCEINLINE Event<void()>& onRelease()     { return m_onRelease; }
  /** @brief Fires when focus is gained */
  NODISCARD FORCEINLINE Event<void()>& onFocusGained() { return m_onFocusGained; }
  /** @brief Fires when focus is lost */
  NODISCARD FORCEINLINE Event<void()>& onFocusLost()   { return m_onFocusLost; }

 protected:
  SceneNode*   m_owner;
  sf::Vector2f m_size        = {100.f, 30.f};
  Anchors      m_anchors;
  sf::Vector2f m_pivot       = {0.f, 0.f};
  sf::Vector2f m_offset      = {0.f, 0.f};
  bool         m_interactable  = true;
  bool         m_hovered       = false;
  bool         m_pressed       = false;
  bool         m_focused       = false;
  bool         m_focusable     = false;
  bool         m_consumesInput = false;

  NavigationMode m_navMode  = NavigationMode::Automatic;
  UIWidget*      m_navUp    = nullptr;
  UIWidget*      m_navDown  = nullptr;
  UIWidget*      m_navLeft  = nullptr;
  UIWidget*      m_navRight = nullptr;

  Event<void()> m_onClick;
  Event<void()> m_onHoverEnter;
  Event<void()> m_onHoverExit;
  Event<void()> m_onPress;
  Event<void()> m_onRelease;
  Event<void()> m_onFocusGained;
  Event<void()> m_onFocusLost;
};

} // namespace sfmx
