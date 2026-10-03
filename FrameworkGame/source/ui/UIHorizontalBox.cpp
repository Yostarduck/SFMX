#include "ui/UIHorizontalBox.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"

namespace sfmx
{

/** @brief  Constructor (normally called through ui::createWidget<UIHorizontalBox>). */
UIHorizontalBox::UIHorizontalBox(sf::Vector2f size)
  : UIWidgetT<UIHorizontalBox, WidgetType::kHorizontalBox>() {
  setSize(size);
}

/** @brief  Type UUID for serialization. */
UUID UIHorizontalBox::getTypeId() const {
  return TypeTraits<UIHorizontalBox>::getTypeId();
}

// -- Layout -------------------------------------------------------------------

/** @brief  Position all children left-to-right with spacing and padding. */
void UIHorizontalBox::updateLayout() {
  float x = m_padding.x;
  for (auto* child : m_children) {
    child->setPosition({x, m_padding.y});
    x += child->getSize().x + m_spacing;
  }
  m_layoutDirty = false;
}

// -- Overrides for hierarchy support ------------------------------------------

/** @brief  Translate children by this box's position so they are relative to the box origin. */
sf::Transform UIHorizontalBox::getChildTransform() const {
  sf::Transform t;
  t.translate(getPosition());
  return t;
}

void UIHorizontalBox::onUpdate(float deltaTime) {
  SFMX_PARAMETER_UNUSED(deltaTime);
  if (m_layoutDirty) {
    updateLayout();
  }
}

/** @brief  Draw the background rectangle. */
void UIHorizontalBox::onDraw(sf::RenderTarget& target,
                              sf::RenderStates states) const {
  sf::RectangleShape bg;
  bg.setSize(getSize());
  bg.setPosition(getPosition());
  bg.setFillColor(m_boxColor);
  target.draw(bg, states);
}

// -- Serialization ------------------------------------------------------------

/** @brief  Serialize flags, rect, colour, spacing, padding. */
void UIHorizontalBox::onSerialize(DataStream& stream) const {
  // Version 2: shared base state (flags/rect/colour, now also anchors + name)
  // moved into UIWidget::serializeBase.
  constexpr uint32 kVersion = 2;
  stream << kVersion;
  serializeBase(stream);

  stream << m_spacing;
  stream << m_padding.x << m_padding.y;
  stream << m_boxColor.r << m_boxColor.g << m_boxColor.b << m_boxColor.a;
}

/** @brief  Restore state written by onSerialize. */
void UIHorizontalBox::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != 2) return;
  deserializeBase(stream);

  stream >> m_spacing;
  stream >> m_padding.x >> m_padding.y;
  stream >> m_boxColor.r >> m_boxColor.g >> m_boxColor.b >> m_boxColor.a;
}

} // namespace sfmx
