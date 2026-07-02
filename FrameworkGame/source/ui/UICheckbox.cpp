#include "ui/UICheckbox.h"
#include "core/DataStream.h"

#include <cmath>

namespace sfmx
{

UICheckbox::UICheckbox(sf::Vector2f size)
  : UIWidgetT<UICheckbox, WidgetType::kCheckbox>(),
    ComponentT<UICheckbox>(nullptr) {
  setSize(size);
  syncColliderToRect();
}

UICheckbox::UICheckbox(SceneNode* node, sf::Vector2f size)
  : UIWidgetT<UICheckbox, WidgetType::kCheckbox>(),
    ComponentT<UICheckbox>(node) {
  setSize(size);
  syncColliderToRect();
}

UICheckbox::~UICheckbox() = default;

UUID UICheckbox::getTypeId() const {
  return TypeTraits<UICheckbox>::getTypeId();
}

void UICheckbox::setChecked(bool checked) {
  if (m_checked != checked) {
    m_checked = checked;
    m_onValueChangedEvent(checked);
  }
}

void UICheckbox::onPointerEnter(sf::Vector2f position) {
  m_hovered = true;
  UIWidget::onPointerEnter(position);
}

void UICheckbox::onPointerExit(sf::Vector2f position) {
  m_hovered = false;
  UIWidget::onPointerExit(position);
}

void UICheckbox::onPointerClick(sf::Vector2f position) {
  setChecked(!m_checked);
  UIWidget::onPointerClick(position);
}

void UICheckbox::onDraw(sf::RenderTarget& target,
                         sf::RenderStates states) const {
  if (!UIWidget::s_canvasDrawing) return;
  if (!isVisible()) {
    return;
  }

  const sf::Vector2f pos = getPosition();
  const sf::Vector2f size = getSize();

  if (m_checked) {
    m_box.setFillColor(m_checkedBoxColor);
  } else if (m_hovered) {
    m_box.setFillColor(m_hoveredBoxColor);
  } else {
    m_box.setFillColor(m_boxColor);
  }
  m_box.setSize(size);
  m_box.setPosition(pos);
  target.draw(m_box, states);

  if (m_checked) {
    constexpr float thickness = 3.f;
    const float pad = size.x * 0.2f;
    const float areaLeft = pos.x + pad;
    const float areaTop = pos.y + pad;
    const float areaSize = size.x - pad * 2.f;

    const sf::Vector2f start{areaLeft + areaSize * 0.15f, areaTop + areaSize * 0.6f};
    const sf::Vector2f corner{areaLeft + areaSize * 0.4f, areaTop + areaSize * 0.75f};
    const sf::Vector2f end{areaLeft + areaSize * 0.85f, areaTop + areaSize * 0.25f};

    const sf::Vector2f d1 = corner - start;
    const float len1 = std::sqrt(d1.x * d1.x + d1.y * d1.y);
    const float angle1 = std::atan2(d1.y, d1.x) * 180.f / 3.14159265f;

    m_checkLine1.setSize({len1, thickness});
    m_checkLine1.setOrigin({0.f, thickness * 0.5f});
    m_checkLine1.setPosition(start);
    m_checkLine1.setRotation(sf::degrees(angle1));
    m_checkLine1.setFillColor(m_checkColor);
    target.draw(m_checkLine1, states);

    const sf::Vector2f d2 = end - corner;
    const float len2 = std::sqrt(d2.x * d2.x + d2.y * d2.y);
    const float angle2 = std::atan2(d2.y, d2.x) * 180.f / 3.14159265f;

    m_checkLine2.setSize({len2, thickness});
    m_checkLine2.setOrigin({0.f, thickness * 0.5f});
    m_checkLine2.setPosition(corner);
    m_checkLine2.setRotation(sf::degrees(angle2));
    m_checkLine2.setFillColor(m_checkColor);
    target.draw(m_checkLine2, states);
  }
}

void UICheckbox::onSerialize(DataStream& stream) const {
  constexpr uint32 kVersion = 1;
  stream << kVersion;

  uint8 flags = 0;
  if (isEnabled())       flags |= 1 << 0;
  if (isVisible())       flags |= 1 << 1;
  if (isInteractable())  flags |= 1 << 2;
  if (isFocused())       flags |= 1 << 3;
  if (isBlockingInput()) flags |= 1 << 4;
  if (m_checked)         flags |= 1 << 5;
  stream << flags;

  const sf::FloatRect& r = getRect();
  stream << r.position.x << r.position.y << r.size.x << r.size.y;

  stream << getAnchorMin().x << getAnchorMin().y
         << getAnchorMax().x << getAnchorMax().y
         << getPivot().x     << getPivot().y;

  const sf::Color& c = getColor();
  stream << c.r << c.g << c.b << c.a;

  stream << m_boxColor.r << m_boxColor.g << m_boxColor.b << m_boxColor.a;
  stream << m_hoveredBoxColor.r << m_hoveredBoxColor.g
         << m_hoveredBoxColor.b << m_hoveredBoxColor.a;
  stream << m_checkedBoxColor.r << m_checkedBoxColor.g
         << m_checkedBoxColor.b << m_checkedBoxColor.a;
  stream << m_checkColor.r << m_checkColor.g << m_checkColor.b << m_checkColor.a;
}

void UICheckbox::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != 1) {
    return;
  }

  uint8 flags = 0;
  stream >> flags;
  setEnabled((flags & (1 << 0)) != 0);
  setVisible((flags & (1 << 1)) != 0);
  setInteractable((flags & (1 << 2)) != 0);
  setFocused((flags & (1 << 3)) != 0);
  setBlocksInput((flags & (1 << 4)) != 0);
  m_checked = (flags & (1 << 5)) != 0;

  sf::FloatRect r;
  stream >> r.position.x >> r.position.y >> r.size.x >> r.size.y;
  setRect(r);

  sf::Vector2f val;
  stream >> val.x >> val.y; setAnchorMin(val);
  stream >> val.x >> val.y; setAnchorMax(val);
  stream >> val.x >> val.y; setPivot(val);

  uint8 cr, cg, cb, ca;
  stream >> cr >> cg >> cb >> ca;
  setColor(sf::Color(cr, cg, cb, ca));

  stream >> m_boxColor.r >> m_boxColor.g >> m_boxColor.b >> m_boxColor.a;
  stream >> m_hoveredBoxColor.r >> m_hoveredBoxColor.g
         >> m_hoveredBoxColor.b >> m_hoveredBoxColor.a;
  stream >> m_checkedBoxColor.r >> m_checkedBoxColor.g
         >> m_checkedBoxColor.b >> m_checkedBoxColor.a;
  stream >> m_checkColor.r >> m_checkColor.g >> m_checkColor.b >> m_checkColor.a;
}

} // namespace sfmx
