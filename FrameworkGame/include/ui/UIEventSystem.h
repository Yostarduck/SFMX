#pragma once

#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Window.hpp>

#include "utils/Module.h"
#include "ui/Canvas.h"

namespace sfmx
{

class InputAction;

/**
 * @brief Singleton module that drives the UI event loop.
 *
 * During its update() phase UIEventSystem:
 * 1. Validates the current selection (deselects destroyed / disabled widgets).
 * 2. Scans registered canvases (sorted by depth) for a widget under the
 *    pointer.
 * 3. Fires enter / exit / down / up / click callbacks as appropriate.
 * 4. Processes keyboard/gamepad navigation via an InputAction.
 *
 * Navigation actions (set via setNavigateAction / setSubmitAction /
 * setCancelAction) follow the existing Mapping system — the user creates
 * actions in their ActionMap and passes pointers here.
 *
 * Canvases auto-register and auto-unregister through @ref CanvasComponent
 * (or manually via registerCanvas / unregisterCanvas).
 *
 * Hit-test coordinate flow:
 *   screen pixel (Mouse::getPosition)
 *     → canvas-local (transform inverse)
 *       → widget-local (widget containsPoint)
 */
class UIEventSystem final : public Module<UIEventSystem>
{
 public:
  using Module::Module;

  // -- Module lifecycle ------------------------------------------------------

  void onStartUp() override;
  void onShutDown() override;
  void update(const sf::WindowBase& window, float deltaTime);

  // -- Canvas registry -------------------------------------------------------

  void registerCanvas(Canvas* canvas);
  void unregisterCanvas(Canvas* canvas);

  // -- Selection -------------------------------------------------------------

  /** @brief The widget currently selected (focused), or nullptr. */
  UIWidget* getSelected() const { return m_selected; }

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
  void setNavigateAction(InputAction* action) { m_navigateAction = action; }

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
    sf::Vector2f canvasPos;   // Transformed into canvas-local space
    UIWidget*   hovered = nullptr;  // Widget currently under the pointer
    UIWidget*   pressed = nullptr;  // Widget on which down was pressed
    bool        buttonDown = false;
  };

  NODISCARD const PointerState& getPointerState() const { return m_pointer; }

 private:
  void validateSelection();
  void processPointer(const sf::WindowBase& window);
  void processNavigation(float deltaTime);
  void moveSelection(const sf::Vector2f& direction);
  void selectFirst();
  UIWidget* findSelectableInDirection(UIWidget* from,
                                      const sf::Vector2f& dir) const;

  Vector<Canvas*> m_canvases;  // Owned externally, sorted by depth
  UIWidget* m_selected = nullptr;
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
