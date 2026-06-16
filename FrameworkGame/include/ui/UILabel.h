#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>

#include "scene/Component.h"
#include "ui/UIWidget.h"
#include "utils/TypeTraits.h"

namespace sfmx {

class UILabel : public ComponentT<UILabel>, public UIWidget
{
 public:
  explicit UILabel(SceneNode* owner);

  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  void setText(const String& t)         { m_text = t; }
  const String& getText() const         { return m_text; }
  void setFont(SPtr<sf::Font> f)        { m_font = std::move(f); }
  void setFontSize(unsigned s)          { m_fontSize = s; }
  void setTextColor(sf::Color c)        { m_textColor = c; }
  void setHorizontalAlign(float a)      { m_hAlign = a; }
  void setVerticalAlign(float a)        { m_vAlign = a; }

 private:
  String          m_text;
  SPtr<sf::Font>  m_font;
  unsigned        m_fontSize   = 14;
  sf::Color       m_textColor  = {220, 220, 220};
  float           m_hAlign     = 0.5f;  // 0=left, 0.5=center, 1=right
  float           m_vAlign     = 0.5f;  // 0=top, 0.5=center, 1=bottom
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UILabel)
