#include "ui/UIVerticalBox.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"

namespace sfmx
{

UIVerticalBox::UIVerticalBox(sf::Vector2f size)
  : UIWidgetT<UIVerticalBox, WidgetType::kVerticalBox>(),
    ComponentT<UIVerticalBox>(nullptr) {
  setSize(size);
  syncColliderToRect();
}

UIVerticalBox::UIVerticalBox(SceneNode* node, sf::Vector2f size)
  : UIWidgetT<UIVerticalBox, WidgetType::kVerticalBox>(),
    ComponentT<UIVerticalBox>(node) {
  setSize(size);
  syncColliderToRect();
}

UUID UIVerticalBox::getTypeId() const {
  return TypeTraits<UIVerticalBox>::getTypeId();
}

void UIVerticalBox::updateLayout() {
  float y = m_padding.y;
  for (auto* child : m_children) {
    child->setPosition({m_padding.x, y});
    child->syncColliderToRect();
    y += child->getSize().y + m_spacing;
  }
  m_layoutDirty = false;
}

sf::Transform UIVerticalBox::getChildTransform() const {
  sf::Transform t;
  t.translate({getPosition().x, getPosition().y});
  return t;
}

// UIVerticalBox.cpp (after getChildTransform)

UIWidget* UIVerticalBox::hitTestInHierarchy(sf::Vector2f point) const {
  if (!isEnabled() || !isVisible() || !isInteractable()) return nullptr;
  if (!containsPoint(point)) return nullptr;

  const sf::Vector2f localPt = point - getPosition();

  for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
    if (UIWidget* hit = (*it)->hitTestInHierarchy(localPt)) {
      return hit;
    }
  }

  return isBlockingInput() ? const_cast<UIVerticalBox*>(this) : nullptr;
}

sf::Vector2f UIVerticalBox::toLocalSpace(sf::Vector2f canvasPoint) const {
  if (m_parent) {
    canvasPoint = m_parent->toLocalSpace(canvasPoint);
  }
  return canvasPoint - getPosition();
}

void UIVerticalBox::onDraw(sf::RenderTarget& target,
                            sf::RenderStates states) const {
  if (!UIWidget::s_canvasDrawing) return;
  if (m_layoutDirty) {
    const_cast<UIVerticalBox*>(this)->updateLayout();
  }

  sf::RectangleShape bg;
  bg.setSize(getSize());
  bg.setPosition(getPosition());
  bg.setFillColor(m_boxColor);
  target.draw(bg, states);
}

void UIVerticalBox::onSerialize(DataStream& stream) const {
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

  stream << m_spacing;
  stream << m_padding.x << m_padding.y;
  stream << m_boxColor.r << m_boxColor.g << m_boxColor.b << m_boxColor.a;
}

void UIVerticalBox::onDeserialize(DataStream& stream) {
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

  stream >> m_spacing;
  stream >> m_padding.x >> m_padding.y;
  stream >> m_boxColor.r >> m_boxColor.g >> m_boxColor.b >> m_boxColor.a;
  syncColliderToRect();
}

} // namespace sfmx
