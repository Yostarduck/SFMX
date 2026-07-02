#pragma once

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include "scene/SceneNode.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

class UISlider final : public UIWidgetT<UISlider, WidgetType::kSlider>,
                       public ComponentT<UISlider>
{
 public:
  using UIWidget::isEnabled;
  using UIWidget::isVisible;
  using UIWidget::isInteractable;
  using UIWidget::setEnabled;
  using UIWidget::setVisible;
  using UIWidget::setInteractable;
  using UIWidget::setFocused;
  using UIWidget::getPosition;
  using UIWidget::setPosition;
  using UIWidget::getSize;
  using UIWidget::setSize;
  using UIWidget::getRect;
  using UIWidget::setRect;
  using UIWidget::getColor;
  using UIWidget::setColor;
  using UIWidget::containsPoint;
  using UIWidget::syncColliderToRect;
  using UIWidget::onPointerDown;
  using UIWidget::onPointerUp;

  UISlider(sf::Vector2f size = {200.f, 20.f});
  UISlider(SceneNode* node, sf::Vector2f size = {200.f, 20.f});
  ~UISlider() override;

  NODISCARD UUID getTypeId() const override;
  void onSerialize(DataStream& stream) const override;
  void onDeserialize(DataStream& stream) override;

  FORCEINLINE float getValue() const { return m_value; }
  void setValue(float value);
  FORCEINLINE float getMinValue() const { return m_minValue; }
  FORCEINLINE float getMaxValue() const { return m_maxValue; }
  void setRange(float min, float max);

  FORCEINLINE void setTrackColor(sf::Color color) { m_trackColor = color; }
  FORCEINLINE void setFillColor(sf::Color color) { m_fillColor = color; }
  FORCEINLINE void setThumbColor(sf::Color color) { m_thumbColor = color; }
  FORCEINLINE void setThumbSize(float size) { m_thumbSize = size; }

  NODISCARD FORCEINLINE sf::Color getTrackColor() const { return m_trackColor; }
  NODISCARD FORCEINLINE sf::Color getFillColor() const { return m_fillColor; }
  NODISCARD FORCEINLINE sf::Color getThumbColor() const { return m_thumbColor; }
  NODISCARD FORCEINLINE float getThumbSize() const { return m_thumbSize; }

  NODISCARD FORCEINLINE HEvent onValueChanged(Function<void(float)> cb) const
    { return m_onValueChangedEvent.connect(std::move(cb)); }

 private:
  void onPointerDown(sf::Vector2f position) override;
  void onPointerUp(sf::Vector2f position) override;
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  void updateValueFromLocalX(float localX);
  NODISCARD float getThumbCenterX() const;

  mutable float m_value = 0.5f;
  float m_minValue = 0.f;
  float m_maxValue = 1.f;
  float m_thumbSize = 16.f;
  mutable bool m_dragging = false;

  mutable sf::RectangleShape m_track;
  mutable sf::RectangleShape m_fill;
  mutable sf::CircleShape m_thumb;

  sf::Color m_trackColor = sf::Color(60, 60, 60);
  sf::Color m_fillColor = sf::Color(100, 150, 255);
  sf::Color m_thumbColor = sf::Color(200, 200, 200);

  Event<void(float)> mutable m_onValueChangedEvent;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UISlider)
