/************************************************************************/
/**
 * @file UILabel.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Text label widget.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>

#include "scene/Component.h"
#include "ui/UIWidget.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief A read-only text label.
 *
 * Draws a single line of text inside the widget rect, aligned by
 * m_hAlign (0 = left, 0.5 = center, 1 = right) and m_vAlign.
 */
class UILabel : public ComponentT<UILabel>, public UIWidget
{
 public:
  /** @brief Construct a label on @p owner */
  explicit UILabel(SceneNode* owner);

  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  // Text

  /** @brief Set the displayed text */
  FORCEINLINE void 
  setText(const String& t)    { m_text = t; }
  /** @brief Current displayed text */
  NODISCARD FORCEINLINE const String& 
  getText() const    { return m_text; }
  /** @brief Set the font */
  FORCEINLINE void setFont(SPtr<sf::Font> f)   { m_font = std::move(f); }
  /** @brief Set the font size in points */
  FORCEINLINE void setFontSize(unsigned s)     { m_fontSize = s; }
  /** @brief Set the text colour */
  FORCEINLINE void setTextColor(sf::Color c)   { m_textColor = c; }

  // Alignment

  /** @brief Horizontal alignment: 0 = left, 0.5 = center, 1 = right */
  FORCEINLINE void setHorizontalAlign(float a) { m_hAlign = a; }
  /** @brief Vertical alignment: 0 = top, 0.5 = center, 1 = bottom */
  FORCEINLINE void setVerticalAlign(float a)   { m_vAlign = a; }

 private:
  String          m_text;
  SPtr<sf::Font>  m_font;
  unsigned        m_fontSize   = 14;
  sf::Color       m_textColor  = {220, 220, 220};
  float           m_hAlign     = 0.5f;
  float           m_vAlign     = 0.5f;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UILabel)
