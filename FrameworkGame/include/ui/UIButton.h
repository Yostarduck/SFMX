/************************************************************************/
/**
 * @file UIButton.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Clickable button widget.
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
 * @brief A rectangular, pressable button with text.
 *
 * Draws a filled rectangle (colour varies by state: idle, hovered,
 * pressed) and centres the label text.  Focus is shown with a 2-px
 * outline.  Fires onClick() when clicked.
 */
class UIButton : public ComponentT<UIButton>, public UIWidget
{
 public:
  /** @brief Construct a button on @p owner */
  explicit UIButton(SceneNode* owner);

  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  // Text

  /** @brief Set the button label */
  FORCEINLINE void setText(const String& t)     { m_text = t; }
  /** @brief Current button label */
  FORCEINLINE const String& getText() const      { return m_text; }
  /** @brief Set the font */
  FORCEINLINE void setFont(SPtr<sf::Font> f)    { m_font = std::move(f); }
  /** @brief Set the font size in points */
  FORCEINLINE void setFontSize(unsigned s)      { m_fontSize = s; }
  /** @brief Set the text colour */
  FORCEINLINE void setTextColor(sf::Color c)    { m_textColor = c; }

  // Colours

  /** @brief Set the idle fill colour */
  FORCEINLINE void setFillColor(sf::Color c)    { m_fillColor = c; }
  /** @brief Set the hover fill colour */
  FORCEINLINE void setHoverColor(sf::Color c)   { m_hoverColor = c; }
  /** @brief Set the pressed fill colour */
  FORCEINLINE void setPressColor(sf::Color c)   { m_pressColor = c; }
  /** @brief Set the focus outline colour */
  FORCEINLINE void setFocusOutlineColor(sf::Color c) { m_focusOutlineColor = c; }
  /** @brief Set the corner radius (not yet implemented) */
  FORCEINLINE void setCornerRadius(float r)     { m_cornerRadius = r; }

 private:
  NODISCARD sf::Color currentFillColor() const;

  String           m_text;
  SPtr<sf::Font>   m_font;
  unsigned         m_fontSize           = 14;
  sf::Color        m_textColor          = {220, 220, 220};
  sf::Color        m_fillColor          = {60, 60, 65};
  sf::Color        m_hoverColor         = {75, 75, 80};
  sf::Color        m_pressColor         = {90, 90, 95};
  sf::Color        m_focusOutlineColor  = {100, 150, 220};
  float            m_cornerRadius       = 0.f;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIButton)
