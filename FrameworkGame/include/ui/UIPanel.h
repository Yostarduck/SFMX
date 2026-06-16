#pragma once

#include <SFML/Graphics/Color.hpp>

#include "scene/Component.h"
#include "ui/UIWidget.h"
#include "utils/TypeTraits.h"

namespace sfmx {

class UIPanel : public ComponentT<UIPanel>, public UIWidget
{
 public:
  explicit UIPanel(SceneNode* owner);

  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  void setFillColor(sf::Color c)   { m_fillColor = c; }
  void setBorderColor(sf::Color c) { m_borderColor = c; }
  void setBorderWidth(float w)     { m_borderWidth = w; }
  void setCornerRadius(float r)    { m_cornerRadius = r; }

 private:
  sf::Color m_fillColor    = {45, 45, 48};
  sf::Color m_borderColor  = {60, 60, 65};
  float     m_borderWidth  = 1.f;
  float     m_cornerRadius = 0.f;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIPanel)
