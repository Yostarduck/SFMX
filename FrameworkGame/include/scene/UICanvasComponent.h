/************************************************************************/
/**
 * @file UICanvasComponent.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Root component for a UI overlay subtree.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/View.hpp>

#include "scene/Component.h"
#include "ui/UITheme.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

class UIWidget;

/**
 * @brief Root component for a UI overlay subtree.
 *
 * Attach to a dedicated SceneNode that is drawn *after* the main scene
 * in the game loop.
 *
 *  - Manages an orthographic view matching the canvas resolution.
 *  - In onDraw() it sets this view so children render in screen-space coords.
 *  - In onUpdate() it resolves layout for all descendant UIWidgets,
 *    processes pointer input (hover / press / click / focus), and
 *    delegates navigation to UIEventSystem.
 *
 * Input is read from Mouse (pointer position/click) directly and from
 * UIEventSystem (Submit/Cancel).  Overlapping widgets are resolved
 * top-to-bottom by draw order; a widget can opt to consume input and
 * block all widgets below it.
 */
class UICanvasComponent : public ComponentT<UICanvasComponent>
{
 public:
  explicit UICanvasComponent(SceneNode* owner);
  ~UICanvasComponent() override;

  // Canvas resolution

  /** @brief Set the virtual canvas size (pixels) */
  void         setCanvasSize(sf::Vector2f size);
  /** @brief Current canvas size */
  sf::Vector2f getCanvasSize() const       { return m_canvasSize; }
  /** @brief The orthographic view used when drawing the UI subtree */
  const sf::View& getView() const           { return m_uiView; }

  // Theme

  /** @brief Mutable reference to the canvas theme */
  UITheme&       getTheme()       { return m_theme; }
  /** @brief Const reference to the canvas theme */
  const UITheme& getTheme() const { return m_theme; }
  /** @brief Replace the theme */
  void           setTheme(const UITheme& t) { m_theme = t; }

  // Focus

  /** @brief Currently focused widget (or nullptr) */
  NODISCARD UIWidget* getFocus() const;

  /** @brief Collect all focusable widgets in the canvas subtree. */
  void collectFocusable(Vector<UIWidget*>& out) const;

  // Component hooks

  void onUpdate(float dt) override;
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

 private:
  void processInput();
  void resolveLayout();
  void resolveWidgetLayout(SceneNode* node, sf::FloatRect parentRect);
  void collectAllHit(SceneNode* node, const sf::Vector2f& mp,
                     Vector<UIWidget*>& outHits) const;

  sf::View     m_uiView;
  sf::Vector2f m_canvasSize = {1920.f, 1080.f};
  UITheme      m_theme;

  Vector<UIWidget*> m_hoveredWidgets;
  Vector<UIWidget*> m_pressedWidgets;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UICanvasComponent)
