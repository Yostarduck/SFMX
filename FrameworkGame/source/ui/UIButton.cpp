#include "ui/UIButton.h"

#include <SFML/Graphics/RectangleShape.hpp>

namespace sfmx
{

UIButton::UIButton(sf::Vector2f size)
  : UIWidgetT<UIButton, WidgetType::kButton>(),
    ComponentT<UIButton>(nullptr) {
  setSize(size);
  syncColliderToRect();
}

UIButton::UIButton(SceneNode* node, sf::Vector2f size)
  : UIWidgetT<UIButton, WidgetType::kButton>(),
    ComponentT<UIButton>(node) {
  setSize(size);
  syncColliderToRect();
}

UIButton::~UIButton() = default;

// -- Type --------------------------------------------------------------------

UUID UIButton::getTypeId() const {
  return TypeTraits<UIButton>::getTypeId();
}

// -- Pointer events ----------------------------------------------------------

void UIButton::onPointerEnter(sf::Vector2f position) {
  m_visualState = VisualState::kHovered;
  UIWidget::onPointerEnter(position);
}

void UIButton::onPointerExit(sf::Vector2f position) {
  m_visualState = VisualState::kNormal;
  UIWidget::onPointerExit(position);
}

void UIButton::onPointerDown(sf::Vector2f position) {
  m_visualState = VisualState::kPressed;
  UIWidget::onPointerDown(position);
}

void UIButton::onPointerUp(sf::Vector2f position) {
  m_visualState = VisualState::kHovered;
  UIWidget::onPointerUp(position);
}

// -- Drawing -----------------------------------------------------------------

void UIButton::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!isVisible()) {
    return;
  }

  sf::RectangleShape shape(getSize());
  shape.setPosition(getPosition());
  shape.setFillColor(resolveColor());
  target.draw(shape, states);
}

sf::Color UIButton::resolveColor() const {
  if (!isEnabled()) {
    return m_disabledColor;
  }

  // Priority: Pressed > Hovered > Focused > Normal
  switch (m_visualState) {
  case VisualState::kPressed:  return m_pressedColor;
  case VisualState::kHovered:  return m_hoveredColor;
  case VisualState::kFocused:
  case VisualState::kDisabled:
  case VisualState::kNormal:   break;
  }

  if (isFocused()) {
    return m_focusedColor;
  }

  return m_normalColor;
}

} // namespace sfmx
