/************************************************************************/
/**
 * @file UIPanel.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Simple filled-rectangle widget for backgrounds and dividers.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/Color.hpp>

#include "scene/Component.h"
#include "ui/UIWidget.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief A non-interactive panel widget that draws a filled rectangle.
 *
 * Useful for backgrounds, separators, or decorative elements.  Does not
 * consume input by default.
 */
class UIPanel : public ComponentT<UIPanel>, public UIWidget
{
 public:
  /** @brief Construct a panel on @p owner */
  explicit UIPanel(SceneNode* owner);

  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  // Visual properties

  /** @brief Set the fill colour */
  FORCEINLINE void setFillColor(sf::Color c)   { m_fillColor = c; }
  /** @brief Set the border colour */
  FORCEINLINE void setBorderColor(sf::Color c) { m_borderColor = c; }
  /** @brief Set the border width in pixels (0 = no border) */
  FORCEINLINE void setBorderWidth(float w)     { m_borderWidth = w; }
  /** @brief Set the corner radius in pixels (not yet implemented) */
  FORCEINLINE void setCornerRadius(float r)    { m_cornerRadius = r; }

 private:
  sf::Color m_fillColor    = {45, 45, 48};
  sf::Color m_borderColor  = {60, 60, 65};
  float     m_borderWidth  = 1.f;
  float     m_cornerRadius = 0.f;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIPanel)
