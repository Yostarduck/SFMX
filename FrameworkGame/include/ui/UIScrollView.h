/************************************************************************/
/**
 * @file UIScrollView.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Scrollable container widget with clipping.
 */
/************************************************************************/
#pragma once

#include "core/platform/Prerequisites.h"
#include <SFML/Graphics/RenderTexture.hpp>

#include "scene/Component.h"
#include "ui/UIWidget.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief A container that clips its children to the widget rect and
 *        scrolls them via an offset.
 *
 * Children are drawn into an internal sf::RenderTexture sized to the
 * viewport.  To prevent double-draw from the parent's SceneNode::draw
 * pass, children are automatically set invisible during onUpdate and
 * drawn manually inside the render texture.
 *
 * @note The scroll view itself must be interactable (or its children
 *       must be) to receive mouse-wheel signals.  Only vertical
 *       scrolling is implemented in this version.
 */
class UIScrollView : public ComponentT<UIScrollView>, public UIWidget
{
 public:
  explicit UIScrollView(SceneNode* owner);

  void onUpdate(float dt) override;
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  // Scroll

  FORCEINLINE void  
  setScrollY(float sy)      { m_scrollY = sy; clampScroll(); }
  NODISCARD FORCEINLINE float 
  getScrollY() const        { return m_scrollY; }
  FORCEINLINE void  
  setContentHeight(float h) { m_contentHeight = h; }
  NODISCARD FORCEINLINE float 
  getContentHeight() const  { return m_contentHeight; }

  /** @brief Scroll speed multiplier for mouse wheel */
  FORCEINLINE void 
  setScrollSpeed(float s) { m_scrollSpeed = s; }
  NODISCARD FORCEINLINE float
  getScrollSpeed() const  { return m_scrollSpeed; }

  // Visual

  FORCEINLINE void 
  setTrackColor(sf::Color c) { m_trackColor = c; }
  FORCEINLINE void 
  setThumbColor(sf::Color c) { m_thumbColor = c; }

 private:
  void clampScroll();
  void drawChildren(sf::RenderTarget& target, sf::RenderStates states,
                    float offsetY) const;

  mutable SPtr<sf::RenderTexture> m_rt;

  float m_scrollY       = 0.f;
  float m_contentHeight = 0.f;
  float m_scrollSpeed   = 200.f;
  float m_thumbSize     = 30.f;

  sf::Color m_trackColor = {50, 50, 55};
  sf::Color m_thumbColor = {120, 120, 130};
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIScrollView)
