#pragma once

#include <SFML/Graphics/RectangleShape.hpp>
#include "scene/SceneNode.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

class UICheckbox final : public UIWidgetT<UICheckbox, WidgetType::kCheckbox>,
                         public ComponentT<UICheckbox>
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
  using UIWidget::onPointerEnter;
  using UIWidget::onPointerExit;
  using UIWidget::onPointerClick;

  UICheckbox(sf::Vector2f size = {24.f, 24.f});
  UICheckbox(SceneNode* node, sf::Vector2f size = {24.f, 24.f});
  ~UICheckbox() override;

  NODISCARD UUID getTypeId() const override;
  void onSerialize(DataStream& stream) const override;
  void onDeserialize(DataStream& stream) override;

  FORCEINLINE bool isChecked() const { return m_checked; }
  void setChecked(bool checked);

  NODISCARD FORCEINLINE HEvent onValueChanged(Function<void(bool)> cb) const
    { return m_onValueChangedEvent.connect(std::move(cb)); }

  FORCEINLINE void setBoxColor(sf::Color color) { m_boxColor = color; }
  FORCEINLINE void setCheckColor(sf::Color color) { m_checkColor = color; }
  FORCEINLINE void setHoveredBoxColor(sf::Color color) { m_hoveredBoxColor = color; }
  FORCEINLINE void setCheckedBoxColor(sf::Color color) { m_checkedBoxColor = color; }

  NODISCARD FORCEINLINE sf::Color getBoxColor() const { return m_boxColor; }
  NODISCARD FORCEINLINE sf::Color getCheckColor() const { return m_checkColor; }
  NODISCARD FORCEINLINE sf::Color getHoveredBoxColor() const { return m_hoveredBoxColor; }
  NODISCARD FORCEINLINE sf::Color getCheckedBoxColor() const { return m_checkedBoxColor; }

 private:
  void onPointerEnter(sf::Vector2f position) override;
  void onPointerExit(sf::Vector2f position) override;
  void onPointerClick(sf::Vector2f position) override;
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  bool m_checked = false;
  bool m_hovered = false;

  mutable sf::RectangleShape m_box;
  mutable sf::RectangleShape m_checkLine1;
  mutable sf::RectangleShape m_checkLine2;

  sf::Color m_boxColor = sf::Color(200, 200, 200);
  sf::Color m_hoveredBoxColor = sf::Color(180, 180, 255);
  sf::Color m_checkedBoxColor = sf::Color(100, 200, 100);
  sf::Color m_checkColor = sf::Color::White;

  Event<void(bool)> mutable m_onValueChangedEvent;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UICheckbox)
