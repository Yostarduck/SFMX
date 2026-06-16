#include "ui/UICanvasComponent.h"

#include "input/Mouse.h"
#include "scene/SceneNode.h"
#include "scene/Transform.h"
#include "ui/UIWidget.h"

namespace sfmx {

UICanvasComponent::UICanvasComponent(SceneNode* owner)
  : ComponentT<UICanvasComponent>(owner)
{
  setCanvasSize(m_canvasSize);
}

void
UICanvasComponent::setCanvasSize(sf::Vector2f size) {
  m_canvasSize = size;
  m_uiView = sf::View(sf::FloatRect({0.f, 0.f}, size));
}

// ── Layout ──────────────────────────────────────────────────────────────────

void
UICanvasComponent::resolveWidgetLayout(SceneNode* node, sf::FloatRect parentRect) {
  UIWidget* widget = nullptr;
  for (Component* c = node->getFirstComponent(); c; c = c->getNextComponent()) {
    widget = dynamic_cast<UIWidget*>(c);
    if (widget) break;
  }

  if (widget) {
    widget->resolveLayout(parentRect);
    parentRect = widget->getWorldRect();
  }

  for (SceneNode* child = node->getFirstChild(); child; child = child->getNextSibling()) {
    resolveWidgetLayout(child, parentRect);
  }
}

void
UICanvasComponent::resolveLayout() {
  resolveWidgetLayout(m_owner, sf::FloatRect{{0.f, 0.f}, m_canvasSize});
}

// ── Input ───────────────────────────────────────────────────────────────────

void
UICanvasComponent::hitTest(SceneNode* node, const sf::Vector2f& mp,
                           UIWidget*& outHit) const {
  // Children first (drawn on top, so they have priority).
  for (SceneNode* child = node->getLastChild(); child; child = child->getPrevSibling()) {
    if (child->isVisible()) hitTest(child, mp, outHit);
    if (outHit) return;
  }

  for (Component* c = node->getFirstComponent(); c; c = c->getNextComponent()) {
    UIWidget* w = dynamic_cast<UIWidget*>(c);
    if (w && w->isInteractable() && w->getWorldRect().contains(mp)) {
      outHit = w;
      return;
    }
  }
}

void
UICanvasComponent::processInput() {
  if (!Mouse::isStarted()) return;
  auto& mouse = Mouse::instance();
  const sf::Vector2f mp{
    static_cast<float>(mouse.getPosition().x),
    static_cast<float>(mouse.getPosition().y)
  };

  // 1. Hit-test top-most interactable widget
  UIWidget* hit = nullptr;
  hitTest(m_owner, mp, hit);

  // 2. Update hover state
  if (hit != m_hoveredWidget) {
    if (m_hoveredWidget) m_hoveredWidget->setHovered(false);
    m_hoveredWidget = hit;
    if (m_hoveredWidget) m_hoveredWidget->setHovered(true);
  }

  // 3. Button state
  const bool leftPressed  = mouse.wasPressedThisFrame(MouseButton::kLeft);
  const bool leftReleased = mouse.wasReleasedThisFrame(MouseButton::kLeft);

  if (leftPressed && m_hoveredWidget) {
    m_pressedWidget = m_hoveredWidget;
    m_pressedWidget->setPressed(true);
  }
  if (leftReleased && m_pressedWidget) {
    m_pressedWidget->setPressed(false);
    m_pressedWidget = nullptr;
  }
}

// ── Hooks ──────────────────────────────────────────────────────────────────

void
UICanvasComponent::onUpdate(float) {
  resolveLayout();
  processInput();
}

void
UICanvasComponent::onDraw(sf::RenderTarget& target, sf::RenderStates) const {
  target.setView(m_uiView);
}

} // namespace sfmx
