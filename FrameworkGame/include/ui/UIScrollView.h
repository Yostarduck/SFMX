/************************************************************************/
/**
 * @file UIScrollView.h
 * @author Swampertor
 * @date 2026/07/11
 * @brief  Scrollable viewport with clipping and content-space children.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/View.hpp>
#include "scene/SceneNode.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

/**
 * @brief  Viewport widget that clips children to its rect and supports
 *         vertical scrolling.
 *
 * Children are positioned in content-space (0,0 = top of the content area).
 * The ScrollView applies its viewport transform and scroll offset during
 * drawing and hit-testing.  Children are NOT registered with the Canvas
 * directly — only the ScrollView root is.
 *
 * Clipping uses sf::View viewport (same technique as UITextBox).
 * Use setScrollOffset() or scrollBy() to programmatically scroll.
 */
class UIScrollView final : public UIWidgetT<UIScrollView, WidgetType::kScrollView>,
                           public ComponentT<UIScrollView>
{
 public:
  /** @brief  Enable/disable state. */
  using UIWidget::isEnabled;
  /** @brief  Visibility flag. */
  using UIWidget::isVisible;
  /** @brief  Whether the widget can receive input. */
  using UIWidget::isInteractable;
  /** @brief  Set enabled/disabled. */
  using UIWidget::setEnabled;
  /** @brief  Show or hide the widget. */
  using UIWidget::setVisible;
  /** @brief  Set interactable (input-receiving) state. */
  using UIWidget::setInteractable;
  /** @brief  Request or release keyboard focus. */
  using UIWidget::setFocused;
  /** @brief  Get position in canvas space. */
  using UIWidget::getPosition;
  /** @brief  Set position in canvas space. */
  using UIWidget::setPosition;
  /** @brief  Get size of the widget. */
  using UIWidget::getSize;
  /** @brief  Set size of the widget. */
  using UIWidget::setSize;
  /** @brief  Get bounding rectangle. */
  using UIWidget::getRect;
  /** @brief  Set bounding rectangle. */
  using UIWidget::setRect;
  /** @brief  Get the tint colour. */
  using UIWidget::getColor;
  /** @brief  Set the tint colour. */
  using UIWidget::setColor;
  /** @brief  Test if a canvas-space point is inside this widget. */
  using UIWidget::containsPoint;
  /** @brief  Sync the physics collider shape with the widget rect. */
  using UIWidget::syncColliderToRect;
  /** @brief  Called when pointer enters the widget area. */
  using UIWidget::onPointerEnter;
  /** @brief  Called when pointer exits the widget area. */
  using UIWidget::onPointerExit;
  /** @brief  Called when the widget is clicked. */
  using UIWidget::onPointerClick;
  /** @brief  Add a child UIWidget. */
  using UIWidget::addChild;
  /** @brief  Remove a child UIWidget. */
  using UIWidget::removeChild;
  /** @brief  Access the list of child widgets. */
  using UIWidget::getChildren;
  /** @brief  Get the parent UIWidget, or nullptr. */
  using UIWidget::getUIparent;

  /** @brief  Standalone constructor (no SceneNode). */
  UIScrollView(sf::Vector2f size = {300.f, 400.f});
  /** @brief  Component constructor attached to a SceneNode. */
  UIScrollView(SceneNode* node, sf::Vector2f size = {300.f, 400.f});
  ~UIScrollView() override = default;

  // -- Serialization ------------------------------------------------------------

  /** @brief  Type UUID for serialization. */
  NODISCARD UUID getTypeId() const override;
  /** @brief  Serialize flags, rect, colour, scroll offset, content height, background colour. */
  void onSerialize(DataStream& stream) const override;
  /** @brief  Restore state written by onSerialize. */
  void onDeserialize(DataStream& stream) override;

  // -- Scrolling ----------------------------------------------------------------

  /** @brief  Vertical scroll offset in pixels (0 = top). */
  FORCEINLINE void setScrollOffset(float offset) { m_scrollOffset = offset; }
  /** @brief  Current vertical scroll offset. */
  NODISCARD FORCEINLINE float getScrollOffset() const { return m_scrollOffset; }

  /** @brief  Scroll by @p delta pixels (positive = down). */
  FORCEINLINE void scrollBy(float delta) { m_scrollOffset += delta; }

  /** @brief  Total height of scrollable content (used for scroll limits). */
  FORCEINLINE void setContentHeight(float height) { m_contentHeight = height; }
  /** @brief  Current content height. */
  NODISCARD FORCEINLINE float getContentHeight() const { return m_contentHeight; }

  /** @brief  Background colour of the viewport area. */
  FORCEINLINE void setBackgroundColor(sf::Color color) { m_backgroundColor = color; }
  /** @brief  Current background colour. */
  NODISCARD FORCEINLINE sf::Color getBackgroundColor() const { return m_backgroundColor; }

  /** @brief  Clamp scroll offset to [0, max] where max = contentHeight - viewport height. */
  void clampScrollOffset();

  // -- Overrides for hierarchy support ----------------------------------------

  /** @brief  Scroll the viewport by the wheel delta (positive = scroll up). */
  void onScroll(float delta) override;
  /** @brief  Translate + scroll offset applied to content-space children. */
  NODISCARD sf::Transform getChildTransform() const override;
  /** @brief  Hit-test with scroll-aware content-space transform. */
  NODISCARD UIWidget* hitTestInHierarchy(sf::Vector2f point) const override;
  /** @brief  Clip to viewport via sf::View, draw background, then draw scrolled children. */
  void drawHierarchy(sf::RenderTarget& target, sf::RenderStates states) const override;
  /** @brief  Convert canvas-space point to content-space, accounting for scroll offset. */
  NODISCARD sf::Vector2f toLocalSpace(sf::Vector2f canvasPoint) const override;

 private:
  // -- Drawing ------------------------------------------------------------------

  /** @brief  Draw the background rectangle covering the viewport area. */
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  /** @brief  Vertical scroll position (0 = top). */
  float m_scrollOffset = 0.f;
  /** @brief  Total content height for clamping scroll limits. */
  float m_contentHeight = 0.f;
  /** @brief  Fill colour drawn behind children in the viewport. */
  sf::Color m_backgroundColor = sf::Color(40, 40, 50, 220);
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIScrollView)
