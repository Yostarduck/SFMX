#include "ui/UIButton.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>

namespace sfmx {

UIButton::UIButton(SceneNode* owner)
  : ComponentT<UIButton>(owner), UIWidget(owner)
{
}

sf::Color
UIButton::currentFillColor() const {
  if (m_pressed) return m_pressColor;
  if (m_hovered) return m_hoverColor;
  return m_fillColor;
}

void
UIButton::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (m_size.x <= 0.f || m_size.y <= 0.f) return;

  // Background
  sf::RectangleShape bg(m_size);
  bg.setFillColor(currentFillColor());
  target.draw(bg, states);

  // Text
  if (m_font && !m_text.empty()) {
    sf::Text txt(*m_font, m_text, m_fontSize);
    txt.setFillColor(m_textColor);
    const sf::FloatRect bounds = txt.getLocalBounds();
    txt.setPosition({
      (m_size.x - bounds.size.x) * 0.5f,
      (m_size.y - bounds.size.y) * 0.5f
    });
    target.draw(txt, states);
  }
}

} // namespace sfmx
