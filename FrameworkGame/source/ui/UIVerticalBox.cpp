/************************************************************************/
/**
 * @file UIVerticalBox.cpp
 * @author Swampertor
 * @date 2026/07/08
 * @brief  Vertical layout container implementation.
 */
/************************************************************************/
#include "ui/UIVerticalBox.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"

namespace sfmx
{

/** @brief  Constructor (normally called through ui::createWidget<UIVerticalBox>). */
UIVerticalBox::UIVerticalBox(sf::Vector2f size)
  : UIWidgetT<UIVerticalBox, WidgetType::kVerticalBox>() {
  setSize(size);
}

/** @brief  Type UUID for serialization. */
UUID UIVerticalBox::getTypeId() const {
  return TypeTraits<UIVerticalBox>::getTypeId();
}

// -- Layout -------------------------------------------------------------------

/** @brief  Position all children top-to-bottom with spacing and padding. */
void UIVerticalBox::updateLayout() {
  float y = m_padding.y;
  for (auto* child : m_children) {
    child->setPosition({m_padding.x, y});
    y += child->getSize().y + m_spacing;
  }
  m_layoutDirty = false;
}

// -- Overrides for hierarchy support ------------------------------------------

/** @brief  Translate children by this box's position so they are relative to the box origin. */
sf::Transform UIVerticalBox::getChildTransform() const {
  sf::Transform t;
  t.translate({getPosition().x, getPosition().y});
  return t;
}

void UIVerticalBox::onUpdate(float deltaTime) {
  SFMX_PARAMETER_UNUSED(deltaTime);
  if (m_layoutDirty) {
    updateLayout();
  }
}

/** @brief  Draw the background rectangle. */
void UIVerticalBox::onDraw(sf::RenderTarget& target,
                            sf::RenderStates states) const {
  sf::RectangleShape bg;
  bg.setSize(getSize());
  bg.setPosition(getPosition());
  bg.setFillColor(m_boxColor);
  target.draw(bg, states);
}

/** @brief  Serialize flags, rect, colour, spacing, padding. */
void UIVerticalBox::onSerialize(DataStream& stream) const {
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
void UIVerticalBox::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != 2) return;
  deserializeBase(stream);

  stream >> m_spacing;
  stream >> m_padding.x >> m_padding.y;
  stream >> m_boxColor.r >> m_boxColor.g >> m_boxColor.b >> m_boxColor.a;
}

} // namespace sfmx
