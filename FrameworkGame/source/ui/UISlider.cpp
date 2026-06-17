#include "ui/UISlider.h"

#include "input/Mouse.h"
#include "scene/SceneNode.h"
#include "scene/Transform.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace sfmx
{

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

UISlider::UISlider(SceneNode* owner)
  : ComponentT<UISlider>(owner), UIWidget(owner)
{
  m_focusable = true;
}

// -----------------------------------------------------------------------------
// Range
// -----------------------------------------------------------------------------

void
UISlider::setRange(float min, float max) {
  m_min = min;
  m_max = std::max(max, min);
  m_value = std::clamp(m_value, m_min, m_max);
}

// -----------------------------------------------------------------------------
// Value
// -----------------------------------------------------------------------------

void
UISlider::setValue(float v) {
  v = std::clamp(v, m_min, m_max);
  if (v != m_value) {
    m_value = v;
    m_onValueChanged(m_value);
  }
}

// -----------------------------------------------------------------------------
// Geometry helpers
// -----------------------------------------------------------------------------

sf::FloatRect
UISlider::trackRect() const {
  return {{m_trackPad, m_size.y * 0.5f - 2.f},
          {m_size.x - m_trackPad * 2.f, 4.f}};
}

sf::Vector2f
UISlider::thumbCenter() const {
  const float t  = (m_value - m_min) / (m_max - m_min);
  const float tx = m_trackPad + t * (m_size.x - m_trackPad * 2.f);
  return {tx, m_size.y * 0.5f};
}

sf::FloatRect
UISlider::thumbRect() const {
  const sf::Vector2f tc = thumbCenter();
  return {{tc.x - m_thumbSize * 0.5f, tc.y - m_thumbSize * 0.5f},
          {m_thumbSize, m_thumbSize}};
}

float
UISlider::valueFromPointer() const {
  const sf::Transform& wt = Component::m_owner->transform().getWorldTransform();
  const sf::Vector2f origin = wt.transformPoint({0.f, 0.f});

  const sf::Vector2i mp = Mouse::instance().getPosition();
  const float localX = static_cast<float>(mp.x) - origin.x;
  const float t = (localX - m_trackPad) / (m_size.x - m_trackPad * 2.f);
  return m_min + std::clamp(t, 0.f, 1.f) * (m_max - m_min);
}

// -----------------------------------------------------------------------------
// Update
// -----------------------------------------------------------------------------

void
UISlider::onUpdate(float) {
  if (!m_pressed) return;
  setValue(valueFromPointer());
}

// -----------------------------------------------------------------------------
// Draw
// -----------------------------------------------------------------------------

void
UISlider::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (m_size.x <= 0.f || m_size.y <= 0.f) return;

  sf::RectangleShape track(trackRect().size);
  track.setPosition(trackRect().position);
  track.setFillColor(m_trackColor);
  target.draw(track, states);

  const sf::Vector2f tc = thumbCenter();
  sf::RectangleShape fill;
  if (tc.x > m_trackPad) {
    fill.setSize({tc.x - m_trackPad, trackRect().size.y});
    fill.setPosition({m_trackPad, trackRect().position.y});
    fill.setFillColor(m_fillColor);
    target.draw(fill, states);
  }

  sf::RectangleShape thumb(thumbRect().size);
  thumb.setPosition(thumbRect().position);
  thumb.setFillColor(m_hovered ? m_thumbHoverColor : m_thumbColor);
  if (m_focused) {
    thumb.setOutlineThickness(2.f);
    thumb.setOutlineColor(m_focusOutlineColor);
  }
  target.draw(thumb, states);
}

} // namespace sfmx
