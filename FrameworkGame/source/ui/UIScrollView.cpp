#include "ui/UIScrollView.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"

#include <algorithm>
#include <cmath>

namespace sfmx
{

/** @brief  Standalone constructor (no SceneNode). */
UIScrollView::UIScrollView(sf::Vector2f size)
  : UIWidgetT<UIScrollView, WidgetType::kScrollView>(),
    ComponentT<UIScrollView>(nullptr) {
  setSize(size);
  syncColliderToRect();
}

/** @brief  Component constructor attached to a SceneNode. */
UIScrollView::UIScrollView(SceneNode* node, sf::Vector2f size)
  : UIWidgetT<UIScrollView, WidgetType::kScrollView>(),
    ComponentT<UIScrollView>(node) {
  setSize(size);
  syncColliderToRect();
}

// -- Serialization ---------------------------------------------------------------

/** @brief  Type UUID for serialization. */
UUID UIScrollView::getTypeId() const {
  return TypeTraits<UIScrollView>::getTypeId();
}

/** @brief  Serialize flags, rect, colour, scroll offset, content height, background colour. */
void UIScrollView::onSerialize(DataStream& stream) const {
  constexpr uint32 kVersion = 1;
  stream << kVersion;

  uint8 flags = 0;
  if (isEnabled())       flags |= 1 << 0;
  if (isVisible())       flags |= 1 << 1;
  if (isInteractable())  flags |= 1 << 2;
  if (isFocused())       flags |= 1 << 3;
  stream << flags;

  const sf::FloatRect& r = getRect();
  stream << r.position.x << r.position.y << r.size.x << r.size.y;

  const sf::Color& c = getColor();
  stream << c.r << c.g << c.b << c.a;

  stream << m_scrollOffset << m_contentHeight;
  stream << m_backgroundColor.r << m_backgroundColor.g
         << m_backgroundColor.b << m_backgroundColor.a;
}

/** @brief  Restore state written by onSerialize. */
void UIScrollView::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != 1) return;

  uint8 flags = 0;
  stream >> flags;
  setEnabled((flags & (1 << 0)) != 0);
  setVisible((flags & (1 << 1)) != 0);
  setInteractable((flags & (1 << 2)) != 0);
  setFocused((flags & (1 << 3)) != 0);

  sf::FloatRect r;
  stream >> r.position.x >> r.position.y >> r.size.x >> r.size.y;
  setRect(r);

  uint8 cr, cg, cb, ca;
  stream >> cr >> cg >> cb >> ca;
  setColor(sf::Color(cr, cg, cb, ca));

  stream >> m_scrollOffset >> m_contentHeight;
  stream >> m_backgroundColor.r >> m_backgroundColor.g
         >> m_backgroundColor.b >> m_backgroundColor.a;
  syncColliderToRect();
}

// -- Scrolling -------------------------------------------------------------------

/** @brief  Clamp scroll offset to [0, max] where max = contentHeight - viewport height. */
void UIScrollView::clampScrollOffset() {
  const float maxOffset = std::max(0.f, m_contentHeight - getSize().y);
  m_scrollOffset = std::clamp(m_scrollOffset, 0.f, maxOffset);
}

// -- Overrides for hierarchy support ---------------------------------------------

/** @brief  Scroll the viewport by the wheel delta (positive = scroll up). */
void UIScrollView::triggerScroll(float delta) {
  scrollBy(-delta * 30.f);
  clampScrollOffset();
}

/** @brief  Translate + scroll offset applied to content-space children. */
sf::Transform UIScrollView::getChildTransform() const {
  sf::Transform t;
  t.translate({getPosition().x, getPosition().y - m_scrollOffset});
  return t;
}

/** @brief  Hit-test with scroll-aware content-space transform. */
UIWidget* UIScrollView::hitTestInHierarchy(sf::Vector2f point) const {
  if (!isEnabled() || !isVisible() || !isInteractable()) return nullptr;
  if (!containsPoint(point)) return nullptr;

  // Transform to content-space for children
  const sf::Vector2f contentPoint = point - getPosition() +
    sf::Vector2f(0.f, m_scrollOffset);

  // Check children in reverse order (last drawn = topmost)
  for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
    UIWidget* child = *it;
    if (UIWidget* hit = child->hitTestInHierarchy(contentPoint)) {
      return hit;
    }
  }

  return isBlockingInput() ? const_cast<UIScrollView*>(this) : nullptr;
}

/** @brief  Clip to viewport via sf::View, draw background, then draw scrolled children. */
void UIScrollView::drawHierarchy(sf::RenderTarget& target,
                                  sf::RenderStates states) const {
  if (!isVisible()) return;

  // Clip to viewport via sf::View
  const sf::View prevView = target.getView();
  const sf::Vector2f targetSize = static_cast<sf::Vector2f>(target.getSize());

  const sf::FloatRect viewportRect(getPosition(), getSize());
  sf::View clipView(viewportRect);
  if (targetSize.x > 0.f && targetSize.y > 0.f) {
    clipView.setViewport(sf::FloatRect(
      {getPosition().x / targetSize.x, getPosition().y / targetSize.y},
      {getSize().x / targetSize.x,     getSize().y / targetSize.y}
    ));
  }
  target.setView(clipView);

  // Draw self (background)
  onDraw(target, states);

  // Draw children with scroll transform
  if (!m_children.empty()) {
    sf::RenderStates childStates = states;
    childStates.transform *= getChildTransform();
    for (auto* child : m_children) {
      child->drawHierarchy(target, childStates);
    }
  }

  target.setView(prevView);
}

/** @brief  Convert canvas-space point to content-space, accounting for scroll offset. */
sf::Vector2f UIScrollView::toLocalSpace(sf::Vector2f canvasPoint) const {
  if (m_parent) {
    canvasPoint = m_parent->toLocalSpace(canvasPoint);
  }
  return canvasPoint - getPosition() + sf::Vector2f(0.f, m_scrollOffset);
}

// -- Drawing ---------------------------------------------------------------------

/** @brief  Draw the background rectangle covering the viewport area. */
void UIScrollView::onDraw(sf::RenderTarget& target,
                           sf::RenderStates states) const {
  if (!UIWidget::s_canvasDrawing) return;

  sf::RectangleShape bg;
  bg.setSize(getSize());
  bg.setPosition(getPosition());
  bg.setFillColor(m_backgroundColor);
  target.draw(bg, states);
}

} // namespace sfmx
