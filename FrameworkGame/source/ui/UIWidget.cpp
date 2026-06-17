#include "ui/UIWidget.h"

#include "scene/SceneNode.h"
#include "scene/Transform.h"

namespace sfmx
{

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

UIWidget::UIWidget(SceneNode* owner)
  : m_owner(owner)
{
}

// -----------------------------------------------------------------------------
// Hover
// -----------------------------------------------------------------------------

void
UIWidget::setHovered(bool v) {
  if (v == m_hovered) return;
  m_hovered = v;
  if (v) m_onHoverEnter();
  else   m_onHoverExit();
}

// -----------------------------------------------------------------------------
// Focus
// -----------------------------------------------------------------------------

void
UIWidget::setFocused(bool v) {
  if (v == m_focused) return;
  m_focused = v;
  if (v) m_onFocusGained();
  else   m_onFocusLost();
}

// -----------------------------------------------------------------------------
// Press / click
// -----------------------------------------------------------------------------

void
UIWidget::setPressed(bool v) {
  if (v == m_pressed) return;
  m_pressed = v;
  if (v) m_onPress();
  else {
    m_onRelease();
    if (m_hovered) m_onClick();
  }
}

// -----------------------------------------------------------------------------
// Geometry
// -----------------------------------------------------------------------------

sf::FloatRect
UIWidget::getWorldRect() const {
  const sf::Transform& wt = m_owner->transform().getWorldTransform();
  const sf::Vector2f pos = wt.transformPoint({0.f, 0.f});
  return {pos, m_size};
}

void
UIWidget::resolveLayout(sf::FloatRect parentRect) {
  const float anchorW = (m_anchors.right - m_anchors.left) * parentRect.size.x;
  const float anchorH = (m_anchors.bottom - m_anchors.top) * parentRect.size.y;

  const float anchorLeft = parentRect.position.x + m_anchors.left * parentRect.size.x;
  const float anchorTop  = parentRect.position.y + m_anchors.top  * parentRect.size.y;

  const float w = (m_anchors.left == m_anchors.right) ? m_size.x : anchorW;
  const float h = (m_anchors.top  == m_anchors.bottom) ? m_size.y : anchorH;

  m_size = {w, h};

  const float posX = anchorLeft + (anchorW - w) * m_pivot.x + m_offset.x;
  const float posY = anchorTop  + (anchorH - h) * m_pivot.y + m_offset.y;

  m_owner->transform().setPosition({posX, posY});
}

} // namespace sfmx
