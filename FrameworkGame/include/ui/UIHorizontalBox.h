/************************************************************************/
/**
 * @file UIHorizontalBox.h
 * @author Swampertor
 * @date 2026/07/08
 * @brief  Horizontal layout container that arranges children left-to-right.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/RectangleShape.hpp>
#include "scene/SceneNode.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

/**
 * @brief  Container that stacks child widgets horizontally with configurable
 *         spacing and padding.
 *
 * Mirrors UIVerticalBox on the X axis.  Children use box-local coordinates.
 * Add via addChild(); the HorizontalBox itself is the only Canvas root.
 */
class UIHorizontalBox final : public UIWidgetT<UIHorizontalBox, WidgetType::kHorizontalBox>,
                              public ComponentT<UIHorizontalBox>
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
  using UIWidget::addChild;
  using UIWidget::removeChild;
  using UIWidget::getChildren;
  using UIWidget::getUIparent;

  /** @brief  Standalone constructor (no SceneNode). */
  UIHorizontalBox(sf::Vector2f size = {400.f, 200.f});
  /** @brief  Component constructor attached to a SceneNode. */
  UIHorizontalBox(SceneNode* node, sf::Vector2f size = {400.f, 200.f});
  ~UIHorizontalBox() override = default;

  /** @brief  Type UUID for serialization. */
  NODISCARD UUID getTypeId() const override;
  void onSerialize(DataStream& stream) const override;
  void onDeserialize(DataStream& stream) override;

  FORCEINLINE void setSpacing(float spacing) { m_spacing = spacing; m_layoutDirty = true; }
  NODISCARD FORCEINLINE float getSpacing() const { return m_spacing; }

  FORCEINLINE void setPadding(const sf::Vector2f& padding) { m_padding = padding; m_layoutDirty = true; }
  NODISCARD FORCEINLINE sf::Vector2f getPadding() const { return m_padding; }

  FORCEINLINE void setBoxColor(sf::Color color) { m_boxColor = color; }
  NODISCARD FORCEINLINE sf::Color getBoxColor() const { return m_boxColor; }

  void updateLayout();

  // -- Overrides for hierarchy support ----------------------------------------

  NODISCARD sf::Transform getChildTransform() const override;
  NODISCARD UIWidget* hitTestInHierarchy(sf::Vector2f point) const override;
  NODISCARD sf::Vector2f toLocalSpace(sf::Vector2f canvasPoint) const override;

 private:
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  float m_spacing = 4.f;
  sf::Vector2f m_padding = {4.f, 4.f};
  sf::Color m_boxColor = sf::Color(50, 50, 60, 200);
  bool m_layoutDirty = false;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIHorizontalBox)
