/************************************************************************/
/**
 * @file UIVerticalBox.h
 * @author Swampertor
 * @date 2026/07/08
 * @brief  Vertical layout container that arranges children top-to-bottom.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/RectangleShape.hpp>
#include "ui/UIWidget.h"

namespace sfmx
{

/**
 * @brief  Container that stacks child widgets vertically with configurable
 *         spacing and padding.
 *
 * Children are positioned in box-local coordinates.  Add children via
 * addChild() — they are owned by the box (destroyed with it).  Call
 * updateLayout() after adding children or changing sizes to recompute
 * positions; it is also called on the next update if the layout is dirty.
 */
class UIVerticalBox final : public UIWidgetT<UIVerticalBox, WidgetType::kVerticalBox>
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
  using UIWidget::onPointerEnter;
  using UIWidget::onPointerExit;
  using UIWidget::onPointerClick;
  using UIWidget::addChild;
  using UIWidget::removeChild;
  using UIWidget::getChildren;
  using UIWidget::getUIparent;

  /** @brief  Constructor (normally called through ui::createWidget<UIVerticalBox>). */
  UIVerticalBox(sf::Vector2f size = {200.f, 400.f});
  ~UIVerticalBox() override = default;

  /** @brief  Type UUID for serialization. */
  NODISCARD UUID getTypeId() const override;
  /** @brief  Serialize flags, rect, colour, spacing, padding. */
  void onSerialize(DataStream& stream) const override;
  /** @brief  Restore state written by onSerialize. */
  void onDeserialize(DataStream& stream) override;

  // -- Layout -----------------------------------------------------------------

  /** @brief  Gap between consecutive children. */
  FORCEINLINE void setSpacing(float spacing) { m_spacing = spacing; m_layoutDirty = true; }
  /** @brief  Current gap between children. */
  NODISCARD FORCEINLINE float getSpacing() const { return m_spacing; }

  /** @brief  Padding inside the box before the first child. */
  FORCEINLINE void setPadding(const sf::Vector2f& padding) { m_padding = padding; m_layoutDirty = true; }
  /** @brief  Current padding. */
  NODISCARD FORCEINLINE sf::Vector2f getPadding() const { return m_padding; }

  /** @brief  Background colour of the box. */
  FORCEINLINE void setBoxColor(sf::Color color) { m_boxColor = color; }
  /** @brief  Current background colour. */
  NODISCARD FORCEINLINE sf::Color getBoxColor() const { return m_boxColor; }

  /** @brief  Recompute child positions.  Call after adding children or resizing. */
  void updateLayout();

  // -- Overrides for hierarchy support ----------------------------------------

  /** @brief  Translate children by this box's position so they are relative to the box origin. */
  NODISCARD sf::Transform getChildTransform() const override;

 private:
  /** @brief  Rebuild layout before drawing if dirty. */
  void onUpdate(float deltaTime) override;
  /** @brief  Draw the background rectangle. */
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  /** @brief  Gap between children in pixels. */
  float m_spacing = 4.f;
  /** @brief  Padding inside the box before the first child. */
  sf::Vector2f m_padding = {4.f, 4.f};
  /** @brief  Background fill colour. */
  sf::Color m_boxColor = sf::Color(50, 50, 60, 200);
  /** @brief  Flag set when layout needs recomputation. */
  bool m_layoutDirty = false;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIVerticalBox)
