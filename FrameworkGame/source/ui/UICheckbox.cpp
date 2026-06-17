#include "ui/UICheckbox.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>

namespace sfmx
{

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

UICheckbox::UICheckbox(SceneNode* owner)
  : ComponentT<UICheckbox>(owner), UIWidget(owner)
{
  m_focusable = true;
  m_consumesInput = true;

  m_clickConn = onClick().connect([this] {
    setChecked(!m_checked);
  });
}

// -----------------------------------------------------------------------------
// State
// -----------------------------------------------------------------------------

void
UICheckbox::setChecked(bool v) {
  if (v == m_checked) return;
  m_checked = v;
  m_onToggle(m_checked);
}

// -----------------------------------------------------------------------------
// Colour helpers
// -----------------------------------------------------------------------------

sf::Color
UICheckbox::currentBoxColor() const {
  if (m_pressed) return m_pressBoxColor;
  if (m_checked) return m_checkedBoxColor;
  if (m_hovered) return m_hoverBoxColor;
  return m_boxColor;
}

// -----------------------------------------------------------------------------
// Draw
// -----------------------------------------------------------------------------

void
UICheckbox::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (m_size.x <= 0.f || m_size.y <= 0.f) return;

  const float bs = std::min(m_boxSize, std::min(m_size.x, m_size.y));

  sf::RectangleShape box({bs, bs});
  box.setFillColor(currentBoxColor());
  box.setOutlineThickness(m_focused ? 2.f : 1.f);
  if (m_pressed)
    box.setOutlineColor(sf::Color::White);
  else if (m_focused)
    box.setOutlineColor(m_focusOutlineColor);
  else
    box.setOutlineColor({100, 100, 105});
  target.draw(box, states);

  if (m_checked) {
    const float pad = bs * 0.2f;
    const float cw  = bs - pad * 2.f;
    const sf::Vector2f center = {bs * 0.5f, bs * 0.5f};

    const float thick = 3.f;
    sf::RectangleShape checkH({cw, thick});
    checkH.setFillColor(m_checkColor);
    checkH.setOrigin({cw * 0.5f, thick * 0.5f});
    checkH.setPosition(center);
    checkH.setRotation(sf::degrees(45.f));
    target.draw(checkH, states);

    sf::RectangleShape checkV({thick, cw});
    checkV.setFillColor(m_checkColor);
    checkV.setOrigin({thick * 0.5f, cw * 0.5f});
    checkV.setPosition(center);
    checkV.setRotation(sf::degrees(45.f));
    target.draw(checkV, states);
  }

  if (m_font && !m_text.empty()) {
    const float labelX = bs + 8.f;
    sf::Text txt(*m_font, m_text, m_fontSize);
    txt.setFillColor(m_textColor);
    if (labelX < m_size.x) {
      txt.setPosition({labelX, (m_size.y - txt.getLocalBounds().size.y) * 0.5f});
      target.draw(txt, states);
    }
  }
}

} // namespace sfmx
