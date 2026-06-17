/************************************************************************/
/**
 * @file UIHBox.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Horizontal layout container widget.
 */
/************************************************************************/
#pragma once

#include "scene/Component.h"
#include "ui/UIWidget.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief Arranges child UIWidgets in a horizontal row.
 *
 * The container sizes itself normally via anchors/pivot.  Children must
 * set their own widths (height is inherited from the container by default).
 * Call @ref arrange() from @c onUpdate to lay out children every frame.
 */
class UIHBox : public ComponentT<UIHBox>, public UIWidget
{
 public:
  explicit UIHBox(SceneNode* owner);

  void onUpdate(float dt) override;
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  /** @brief Gap (pixels) between consecutive children */
  void  setSpacing(float s)   { m_spacing = s; }
  float getSpacing() const    { return m_spacing; }

  /** @brief Padding inset from all edges */
  void  setPadding(float p)   { m_padding = p; }
  float getPadding() const    { return m_padding; }

  /**
   * @brief How leftover horizontal space is distributed.
   *   0.0 = left-align children, 0.5 = center, 1.0 = right-align.
   */
  FORCEINLINE void  
  setChildAlign(float a)  { m_childAlign = a; }
  FORCEINLINE float 
  getChildAlign() const   { return m_childAlign; }

  NODISCARD bool 
  isLayoutContainer() const override { return true; }

 private:
  void arrange();

  float m_spacing    = 4.f;
  float m_padding    = 0.f;
  float m_childAlign = 0.5f;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIHBox)
