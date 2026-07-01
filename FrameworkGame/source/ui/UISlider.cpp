#include "ui/UISlider.h"
#include "ui/UIEventSystem.h"
#include "core/DataStream.h"

namespace sfmx
{

UISlider::UISlider(sf::Vector2f size)
  : UIWidgetT<UISlider, WidgetType::kSlider>(),
    ComponentT<UISlider>(nullptr) {
  setSize(size);
  syncColliderToRect();
}

UISlider::UISlider(SceneNode* node, sf::Vector2f size)
  : UIWidgetT<UISlider, WidgetType::kSlider>(),
    ComponentT<UISlider>(node) {
  setSize(size);
  syncColliderToRect();
}

UISlider::~UISlider() = default;

UUID UISlider::getTypeId() const {
  return TypeTraits<UISlider>::getTypeId();
}

void UISlider::setValue(float value) {
  value = std::max(m_minValue, std::min(m_maxValue, value));
  if (m_value != value) {
    m_value = value;
    m_onValueChangedEvent(value);
  }
}

void UISlider::setRange(float min, float max) {
  m_minValue = min;
  m_maxValue = max;
  setValue(m_value);
}

void UISlider::updateValueFromLocalX(float localX) {
  const float w = getSize().x;
  if (w <= 0.f) {
    return;
  }
  const float t = std::max(0.f, std::min(1.f, localX / w));
  setValue(m_minValue + t * (m_maxValue - m_minValue));
}

float UISlider::getThumbCenterX() const {
  const float w = getSize().x;
  if (w <= 0.f) {
    return 0.f;
  }
  const float t = (m_value - m_minValue) / (m_maxValue - m_minValue);
  return t * w;
}

void UISlider::onPointerDown(sf::Vector2f position) {
  UIWidget::onPointerDown(position);
  updateValueFromLocalX(position.x - getPosition().x);
  m_dragging = true;
}

void UISlider::onPointerUp(sf::Vector2f position) {
  m_dragging = false;
  UIWidget::onPointerUp(position);
}

void UISlider::onDraw(sf::RenderTarget& target,
                       sf::RenderStates states) const {
  if (!UIWidget::s_canvasDrawing) return;
  if (!isVisible()) {
    return;
  }

  if (m_dragging) {
    if (UIEventSystem::isStarted()) {
      const auto& ptr = UIEventSystem::instance().getPointerState();
      if (ptr.buttonDown) {
        const sf::Vector2f localPos = ptr.canvasPos - getPosition();
        const float w = getSize().x;
        if (w > 0.f) {
          const float t = std::max(0.f, std::min(1.f, localPos.x / w));
          const float newValue = m_minValue + t * (m_maxValue - m_minValue);
          if (m_value != newValue) {
            m_value = newValue;
            m_onValueChangedEvent(m_value);
          }
        }
      } else {
        m_dragging = false;
      }
    }
  }

  const sf::Vector2f pos = getPosition();
  const sf::Vector2f size = getSize();
  const float trackHeight = size.y * 0.4f;
  const float trackY = pos.y + (size.y - trackHeight) * 0.5f;
  const float thumbCX = pos.x + getThumbCenterX();

  m_track.setSize({size.x, trackHeight});
  m_track.setPosition({pos.x, trackY});
  m_track.setFillColor(m_trackColor);
  target.draw(m_track, states);

  const float fillWidth = thumbCX - pos.x;
  if (fillWidth > 0.f) {
    m_fill.setSize({fillWidth, trackHeight});
    m_fill.setPosition({pos.x, trackY});
    m_fill.setFillColor(m_fillColor);
    target.draw(m_fill, states);
  }

  m_thumb.setRadius(m_thumbSize * 0.5f);
  m_thumb.setOrigin({m_thumbSize * 0.5f, m_thumbSize * 0.5f});
  m_thumb.setPosition({thumbCX, pos.y + size.y * 0.5f});
  m_thumb.setFillColor(m_thumbColor);
  target.draw(m_thumb, states);
}

void UISlider::onSerialize(DataStream& stream) const {
  constexpr uint32 kVersion = 1;
  stream << kVersion;

  uint8 flags = 0;
  if (isEnabled())       flags |= 1 << 0;
  if (isVisible())       flags |= 1 << 1;
  if (isInteractable())  flags |= 1 << 2;
  if (isFocused())       flags |= 1 << 3;
  if (isBlockingInput()) flags |= 1 << 4;
  stream << flags;

  const sf::FloatRect& r = getRect();
  stream << r.position.x << r.position.y << r.size.x << r.size.y;

  stream << getAnchorMin().x << getAnchorMin().y
         << getAnchorMax().x << getAnchorMax().y
         << getPivot().x     << getPivot().y;

  const sf::Color& c = getColor();
  stream << c.r << c.g << c.b << c.a;

  stream << m_value << m_minValue << m_maxValue;
  stream << m_thumbSize;

  stream << m_trackColor.r << m_trackColor.g << m_trackColor.b << m_trackColor.a;
  stream << m_fillColor.r << m_fillColor.g << m_fillColor.b << m_fillColor.a;
  stream << m_thumbColor.r << m_thumbColor.g << m_thumbColor.b << m_thumbColor.a;
}

void UISlider::onDeserialize(DataStream& stream) {
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

  stream >> m_value >> m_minValue >> m_maxValue;
  stream >> m_thumbSize;
  setValue(m_value);

  uint8 tr, tg, tb, ta;
  stream >> tr >> tg >> tb >> ta; m_trackColor = sf::Color(tr, tg, tb, ta);
  stream >> tr >> tg >> tb >> ta; m_fillColor = sf::Color(tr, tg, tb, ta);
  stream >> tr >> tg >> tb >> ta; m_thumbColor = sf::Color(tr, tg, tb, ta);
}

} // namespace sfmx
