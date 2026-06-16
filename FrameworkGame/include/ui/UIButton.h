#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>

#include "scene/Component.h"
#include "ui/UIWidget.h"
#include "utils/TypeTraits.h"

namespace sfmx {

class UIButton : public ComponentT<UIButton>, public UIWidget
{
 public:
  explicit UIButton(SceneNode* owner);

  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  void setText(const String& t)         { m_text = t; }
  const String& getText() const         { return m_text; }
  void setFont(SPtr<sf::Font> f)        { m_font = std::move(f); }
  void setFontSize(unsigned s)          { m_fontSize = s; }
  void setTextColor(sf::Color c)        { m_textColor = c; }
  void setFillColor(sf::Color c)        { m_fillColor = c; }
  void setHoverColor(sf::Color c)       { m_hoverColor = c; }
  void setPressColor(sf::Color c)       { m_pressColor = c; }
  void setCornerRadius(float r)         { m_cornerRadius = r; }

 private:
  NODISCARD sf::Color currentFillColor() const;

  String          m_text;
  SPtr<sf::Font>  m_font;
  unsigned        m_fontSize     = 14;
  sf::Color       m_textColor    = {220, 220, 220};
  sf::Color       m_fillColor    = {60, 60, 65};
  sf::Color       m_hoverColor   = {75, 75, 80};
  sf::Color       m_pressColor   = {90, 90, 95};
  float           m_cornerRadius = 0.f;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIButton)
