/************************************************************************/
/**
 * @file UIScrollView.h
 * @author Swampertor
 * @date 2026/07/08
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
  UIScrollView(sf::Vector2f size = {300.f, 400.f});
  /** @brief  Component constructor attached to a SceneNode. */
  UIScrollView(SceneNode* node, sf::Vector2f size = {300.f, 400.f});
  ~UIScrollView() override = default;

  /** @brief  Type UUID for serialization. */
  NODISCARD UUID getTypeId() const override;
  void onSerialize(DataStream& stream) const override;
  void onDeserialize(DataStream& stream) override;

  /** @brief  Vertical scroll offset in pixels (0 = top). */
  FORCEINLINE void setScrollOffset(float offset) { m_scrollOffset = offset; }
  /** @brief  Current vertical scroll offset. */
  NODISCARD FORCEINLINE float getScrollOffset() const { return m_scrollOffset; }

  /** @brief  Scroll by @p delta pixels (positive = down). */
  FORCEINLINE void scrollBy(float delta) { m_scrollOffset += delta; }

  /** @brief  Total height of scrollable content (used for scroll limits). */
  FORCEINLINE void setContentHeight(float height) { m_contentHeight = height; }
  NODISCARD FORCEINLINE float getContentHeight() const { return m_contentHeight; }

  /** @brief  Background colour of the viewport area. */
  FORCEINLINE void setBackgroundColor(sf::Color color) { m_backgroundColor = color; }
  NODISCARD FORCEINLINE sf::Color getBackgroundColor() const { return m_backgroundColor; }

  /** @brief  Clamp scroll offset to valid range [0, max]. */
  void clampScrollOffset();

  // -- Overrides for hierarchy support ----------------------------------------

  void onScroll(float delta) override;
  NODISCARD sf::Transform getChildTransform() const override;
  NODISCARD UIWidget* hitTestInHierarchy(sf::Vector2f point) const override;
  void drawHierarchy(sf::RenderTarget& target, sf::RenderStates states) const override;
  NODISCARD sf::Vector2f toLocalSpace(sf::Vector2f canvasPoint) const override;

 private:
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  float m_scrollOffset = 0.f;
  float m_contentHeight = 0.f;
  sf::Color m_backgroundColor = sf::Color(40, 40, 50, 220);
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIScrollView)
