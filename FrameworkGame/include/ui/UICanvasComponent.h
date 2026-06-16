#pragma once

#include <SFML/Graphics/View.hpp>

#include "scene/Component.h"
#include "ui/UITheme.h"
#include "utils/TypeTraits.h"

namespace sfmx {

class UIWidget;

// ── UICanvasComponent ───────────────────────────────────────────────────────
//
// Root component for a UI overlay subtree. Attach to a dedicated SceneNode that
// is drawn *after* the main scene in the game loop.
//
//  - Manages an orthographic view matching the canvas resolution.
//  - In onDraw() it sets this view so children render in screen-space coords.
//  - In onUpdate() it resolves layout for all descendant UIWidgets and
//    processes mouse input (hover / click).
//

class UICanvasComponent : public ComponentT<UICanvasComponent>
{
 public:
  explicit UICanvasComponent(SceneNode* owner);

  // ── Canvas resolution ──────────────────────────────────────────────────
  void         setCanvasSize(sf::Vector2f size);
  sf::Vector2f getCanvasSize() const       { return m_canvasSize; }
  const sf::View& getView() const           { return m_uiView; }

  // ── Theme ──────────────────────────────────────────────────────────────
  UITheme&       getTheme()       { return m_theme; }
  const UITheme& getTheme() const { return m_theme; }
  void           setTheme(const UITheme& t) { m_theme = t; }

  // ── Hooks ──────────────────────────────────────────────────────────────
  void onUpdate(float dt) override;
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

 private:
  void processInput();
  void resolveLayout();
  void resolveWidgetLayout(SceneNode* node, sf::FloatRect parentRect);
  void hitTest(SceneNode* node, const sf::Vector2f& mp, UIWidget*& outHit) const;

  sf::View     m_uiView;
  sf::Vector2f m_canvasSize = {1920.f, 1080.f};
  UITheme      m_theme;

  UIWidget* m_hoveredWidget = nullptr;
  UIWidget* m_pressedWidget = nullptr;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UICanvasComponent)
