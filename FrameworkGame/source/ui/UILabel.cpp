#include "ui/UILabel.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>

namespace sfmx {

UILabel::UILabel(SceneNode* owner)
  : ComponentT<UILabel>(owner), UIWidget(owner)
{
}

void
UILabel::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!m_font || m_text.empty()) return;

  sf::Text txt(*m_font, m_text, m_fontSize);
  txt.setFillColor(m_textColor);

  const sf::FloatRect bounds = txt.getLocalBounds();
  const float ox = (m_size.x - bounds.size.x) * m_hAlign;
  const float oy = (m_size.y - bounds.size.y) * m_vAlign;
  txt.setPosition({ox, oy});

  target.draw(txt, states);
}

} // namespace sfmx
