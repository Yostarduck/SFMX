#include "ui/UIScrollView.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"

#include <algorithm>
#include <cmath>

namespace sfmx
{

/** @brief  Constructor (normally called through ui::createWidget<UIScrollView>). */
UIScrollView::UIScrollView(sf::Vector2f size)
  : UIWidgetT<UIScrollView, WidgetType::kScrollView>() {
  setSize(size);
}

// -- Serialization ---------------------------------------------------------------

/** @brief  Type UUID for serialization. */
UUID UIScrollView::getTypeId() const {
  return TypeTraits<UIScrollView>::getTypeId();
}

/** @brief  Serialize flags, rect, colour, scroll offset, content height, background colour. */
void UIScrollView::onSerialize(DataStream& stream) const {
  // Version 2: shared base state (flags/rect/colour, now also anchors + name)
  // moved into UIWidget::serializeBase.
  constexpr uint32 kVersion = 2;
  stream << kVersion;
  serializeBase(stream);

  stream << m_scrollOffset << m_contentHeight;
  stream << m_backgroundColor.r << m_backgroundColor.g
         << m_backgroundColor.b << m_backgroundColor.a;
}

/** @brief  Restore state written by onSerialize. */
void UIScrollView::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != 2) return;
  deserializeBase(stream);

  stream >> m_scrollOffset >> m_contentHeight;
  stream >> m_backgroundColor.r >> m_backgroundColor.g
         >> m_backgroundColor.b >> m_backgroundColor.a;
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
  sf::RectangleShape bg;
  bg.setSize(getSize());
  bg.setPosition(getPosition());
  bg.setFillColor(m_backgroundColor);
  target.draw(bg, states);
}

} // namespace sfmx
