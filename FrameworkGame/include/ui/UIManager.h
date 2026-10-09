/************************************************************************/
/**
 * @file UIManager.h
 * @author Swampertor
 * @date 2026/06/10
 * @brief  Singleton module owning the whole widget tree: input, update, draw.
 */
/************************************************************************/
#pragma once

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Window.hpp>

#include "utils/Module.h"
#include "ui/UIWidget.h"

namespace sfmx
{

class InputAction;
class UITextBox;

/**
 * @brief Singleton module that owns and drives the whole UI layer.
 *
 * Responsibilities:
 *  - Owns the ROOT widgets (insertion order = draw order; hit-tested in
 *    reverse, so the last-added root is on top). Roots are destroyed by the
 *    manager at shutDown; children are destroyed by their parent.
 *  - Registers the widget memory pools at start-up (widgets are always
 *    pool-allocated via `ui::createWidget<T>`).
 *  - update(): validateSelection → pointer → scroll → navigation → recursive
 *    widget onUpdate.
 *  - draw(): sets the default view (window-pixel coordinates) and draws every
 *    root hierarchy; call it AFTER the post-processing chain so the HUD stays
 *    crisp.
 *  - findByName(): recursive name lookup over all roots (`UI:get(name)` in Lua).
 *
 * Hit-test coordinate flow (no canvas transform):
 *   screen pixel (Mouse::getPosition)
 *     → root space (identity)
 *       → widget-local (widget containsPoint)
 */
class UIManager final : public Module<UIManager>
{
 public:
  using Module::Module;

  // -- Module lifecycle ------------------------------------------------------

  void onStartUp() override;
  void onShutDown() override;

  /**
   * @brief Drive UI input + widget updates for this frame.
   *
   * Call once per frame before the scene update.
   */
  void update(const sf::WindowBase& window, float deltaTime);

  /**
   * @brief Draw every root widget hierarchy.
   *
   * Saves the target's view, switches to the default view (window pixels —
   * matching the hit-test coordinate space), draws, then restores. Call after
   * the post-processing chain has rendered the scene.
   */
  void draw(sf::RenderTarget& target);

  /**
   * @brief Route a window event through the UI layer.
   *
   * Handles text editing for the focused UITextBox (TextEntered, caret keys,
   * Escape-to-cancel) and returns true when the event was consumed — the
   * caller must then NOT forward it to the game input layer, so keystrokes
   * meant for an editor never leak into gameplay bindings.
   * Returns false for everything else (pointer input is polled in update()).
   */
  bool handleEvent(const sf::Event& event);

  /**
   * @brief Re-apply every root widget's slot against @p viewportSize.
   *
   * The implicit viewport-spanning root panel: called from the
   * sf::Event::Resized branch of handleEvent (and by addRoot while the
   * viewport is already known). Default anchors leave positions unchanged.
   */
  void relayout(const sf::Vector2f& viewportSize);

  // -- UI documents ----------------------------------------------------------

  /**
   * @brief Save every root (and its subtree) as a UI document to @p path.
   * @return False when the blob could not be built or the file not written.
   */
  bool saveUI(const String& path);

  /**
   * @brief Load a UI document and ADD its roots to the manager.
   *
   * Appends — existing roots stay (remove them first for a full replace).
   * On failure nothing is attached. Requires the widget registry + pools,
   * both registered by onStartUp.
   */
  bool loadUI(const String& path);

  // -- Root registry ---------------------------------------------------------

  /**
   * @brief Register a widget as a root (drawn/hit-tested at the top level).
   *
   * The manager takes ownership: the root is destroyed at shutDown. The widget
   * must not be a child of another widget.
   */
  void addRoot(UIWidget* widget);

  /**
   * @brief Remove @p widget from the roots without destroying it.
   * @return True if the widget was found and removed.
   */
  bool removeRoot(UIWidget* widget);

  /** @brief All roots in insertion (draw) order. */
  NODISCARD FORCEINLINE const Vector<UIWidget*>& getRoots() const { return m_roots; }

  /** @brief First root/child (DFS) named @p name, or nullptr. */
  NODISCARD UIWidget* findByName(StringView name) const;

  // -- Selection -------------------------------------------------------------

  /** @brief The widget currently selected (focused), or nullptr. */
  NODISCARD FORCEINLINE UIWidget* getSelected() const { return m_selected; }

  /**
   * @brief Set the selected (focused) widget.
   *
   * Fires onDeselect on the previous selection and onSelect on the new one.
   * Pass nullptr to clear selection.
   */
  void setSelected(UIWidget* widget);

  // -- InputAction integration -----------------------------------------------

  /**
   * @brief Set the navigate action (Axis2D — reads as Vector2).
   *
   * The system polls getValue() each frame; no subscription needed.
   * Pass nullptr to disable keyboard/gamepad navigation.
   */
  FORCEINLINE void setNavigateAction(InputAction* action) { m_navigateAction = action; }

  /**
   * @brief Set the submit action (Button — fires once per press).
   *
   * The system subscribes to onPerformed.  Pass nullptr to disable.
   */
  void setSubmitAction(InputAction* action);

  /**
   * @brief Set the cancel action (Button — fires once per press).
   *
   * The system subscribes to onPerformed.  Pass nullptr to disable.
   */
  void setCancelAction(InputAction* action);

  // -- Pointer state (public for debugging / extensions) ---------------------

  struct PointerState
  {
    sf::Vector2i screenPos;   // Raw window-pixel coordinates
    sf::Vector2f canvasPos;   // Root space (window pixels; kept name for slider code)
    UIWidget*   hovered = nullptr;  // Widget currently under the pointer
    UIWidget*   pressed = nullptr;  // Widget on which down was pressed
    bool        buttonDown = false;
  };

  NODISCARD FORCEINLINE const PointerState& getPointerState() const { return m_pointer; }

  /**
   * @brief Topmost widget under @p point (root space), honouring the
   *        blocking / fall-through contract: roots are probed newest-first,
   *        a blocking hit wins immediately, a non-blocking hit is only the
   *        fallback. Public so tests and tools can probe the tree headlessly
   *        without injecting pointer input (normal dispatch runs through
   *        processPointer).
   */
  NODISCARD UIWidget* hitTest(sf::Vector2f point) const;

  /**
   * @brief Move keyboard/gamepad selection one step toward @p direction —
   *        explicit nav links first, then scored search, selectFirst when
   *        there is no (or a dead) selection. Public so tests can drive
   *        navigation without the InputAction poll.
   */
  void moveSelection(const sf::Vector2f& direction);

 private:
  friend class UIWidget;

  /**
   * @brief Called from ~UIWidget: clear manager-side references to a widget
   *        that is being destroyed (selection fires deselect; hover/press are
   *        dropped silently so no event reaches a dying widget).
   */
  void forgetWidget(UIWidget* widget);

  /** @brief Recursively run onUpdate on @p widget and its children. */
  void updateHierarchy(UIWidget* widget, float deltaTime);

  void validateSelection();

  /** @brief The focused text editor, or nullptr when the selection isn't one. */
  NODISCARD UITextBox* activeTextEditor() const;

  void processPointer(const sf::WindowBase& window);
  void processScroll();
  void processNavigation(float deltaTime);
  void selectFirst();
  UIWidget* findSelectableInDirection(UIWidget* from,
                                      const sf::Vector2f& dir) const;

  Vector<UIWidget*> m_roots;   // Owned; insertion order = draw order
  UIWidget* m_selected = nullptr;
  sf::Vector2f m_viewportSize{0.f, 0.f};  // root-slot space; {0,0} = no Resized yet
  PointerState m_pointer;

  // InputAction integration
  InputAction* m_navigateAction = nullptr;
  InputAction* m_submitAction = nullptr;
  InputAction* m_cancelAction = nullptr;
  HEvent m_submitSub;
  HEvent m_cancelSub;

  // Navigation state
  float m_navTimer = 0.f;
  bool m_navHeld = false;
};

} // namespace sfmx
