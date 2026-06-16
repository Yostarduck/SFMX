#include "ui/UIPanel.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace sfmx {

UIPanel::UIPanel(SceneNode* owner)
  : ComponentT<UIPanel>(owner), UIWidget(owner)
{
}

void
UIPanel::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (m_size.x <= 0.f || m_size.y <= 0.f) return;

  sf::RectangleShape rect(m_size);
  rect.setFillColor(m_fillColor);
  if (m_borderWidth > 0.f) {
    rect.setOutlineColor(m_borderColor);
    rect.setOutlineThickness(m_borderWidth);
  }
  target.draw(rect, states);
}

} // namespace sfmx
