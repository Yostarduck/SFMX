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
  /** @brief  Delegate to UIWidget::onPointerEnter. */
  using UIWidget::onPointerEnter;
  /** @brief  Delegate to UIWidget::onPointerExit. */
  using UIWidget::onPointerExit;
  /** @brief  Delegate to UIWidget::onPointerClick. */
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

  // -- Serialization ------------------------------------------------------------

  /** @brief  Type UUID for serialization. */
  NODISCARD UUID getTypeId() const override;
  /** @brief  Serialize flags, rect, colour, spacing, padding. */
  void onSerialize(DataStream& stream) const override;
  /** @brief  Restore state written by onSerialize. */
  void onDeserialize(DataStream& stream) override;

  // -- Layout -------------------------------------------------------------------

  /** @brief  Spacing between child widgets. */
  FORCEINLINE void setSpacing(float spacing) { m_spacing = spacing; m_layoutDirty = true; }
  /** @brief  Current spacing value. */
  NODISCARD FORCEINLINE float getSpacing() const { return m_spacing; }

  /** @brief  Padding around child widgets. */
  FORCEINLINE void setPadding(const sf::Vector2f& padding) { m_padding = padding; m_layoutDirty = true; }
  /** @brief  Current padding value. */
  NODISCARD FORCEINLINE sf::Vector2f getPadding() const { return m_padding; }

  /** @brief  Override the background fill colour. */
  FORCEINLINE void setBoxColor(sf::Color color) { m_boxColor = color; }
  /** @brief  Current background fill colour. */
  NODISCARD FORCEINLINE sf::Color getBoxColor() const { return m_boxColor; }

  /** @brief  Position all children left-to-right with spacing and padding. */
  void updateLayout();

  // -- Overrides for hierarchy support ----------------------------------------

  /** @brief  Translate children by this box's position so they are relative to the box origin. */
  NODISCARD sf::Transform getChildTransform() const override;
  /** @brief  Recursive hit-test: transform point to local space, check children in reverse order. */
  NODISCARD UIWidget* hitTestInHierarchy(sf::Vector2f point) const override;
  /** @brief  Convert canvas-space point to box-local space, walking the parent chain. */
  NODISCARD sf::Vector2f toLocalSpace(sf::Vector2f canvasPoint) const override;

 private:
  /** @brief  Draw the background rectangle.  Auto-calls updateLayout if layout is dirty. */
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  /** @brief  Spacing between child widgets. */
  float m_spacing = 4.f;
  /** @brief  Padding around child widgets. */
  sf::Vector2f m_padding = {4.f, 4.f};
  /** @brief  Background fill colour. */
  sf::Color m_boxColor = sf::Color(50, 50, 60, 200);
  /** @brief  Whether layout needs to be recalculated. */
  bool m_layoutDirty = false;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIHorizontalBox)
