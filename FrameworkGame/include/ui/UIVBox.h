/************************************************************************/
/**
 * @file UIVBox.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Vertical layout container widget.
 */
/************************************************************************/
#pragma once

#include "scene/Component.h"
#include "ui/UIWidget.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief Arranges child UIWidgets in a vertical column.
 *
 * The container sizes itself normally via anchors/pivot.  Children must
 * set their own heights (width is inherited from the container by default).
 * Call @ref arrange() from @c onUpdate to lay out children every frame.
 */
class UIVBox : public ComponentT<UIVBox>, public UIWidget
{
 public:
  explicit UIVBox(SceneNode* owner);

  void onUpdate(float dt) override;
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  /** @brief Gap (pixels) between consecutive children */
  FORCEINLINE void 
  setSpacing(float s) { m_spacing = s; }
  NODISCARD FORCEINLINE float 
  getSpacing() const  { return m_spacing; }

  /** @brief Padding inset from all edges */
  FORCEINLINE void 
  setPadding(float p) { m_padding = p; }
  NODISCARD FORCEINLINE float 
  getPadding() const  { return m_padding; }

  /**
   * @brief How leftover vertical space is distributed.
   *   0.0 = top-align children, 0.5 = center, 1.0 = bottom-align.
   */
  FORCEINLINE void 
  setChildAlign(float a)  { m_childAlign = a; }
  NODISCARD FORCEINLINE float 
  getChildAlign() const   { return m_childAlign; }

  NODISCARD FORCEINLINE bool 
  isLayoutContainer() const override { return true; }

 private:
  void arrange();

  float m_spacing    = 4.f;
  float m_padding    = 0.f;
  float m_childAlign = 0.5f;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIVBox)
