#include "ui/UITextBox.h"

#include <algorithm>

#include "input/Keyboard.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>

namespace sfmx
{

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static float
prefixWidth(const sf::Font& font, const sf::String& str, int n, unsigned size) {
  if (n <= 0) return 0.f;
  n = std::min(n, static_cast<int>(str.getSize()));
  sf::String prefix = sf::String::fromUtf32(str.begin(), str.begin() + n);
  sf::Text t(font, prefix, size);
  return t.getLocalBounds().size.x;
}

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

UITextBox::UITextBox(SceneNode* owner)
  : ComponentT<UITextBox>(owner), UIWidget(owner)
{
  m_focusable = true;
  m_consumesInput = true;
}

// -----------------------------------------------------------------------------
// Text
// -----------------------------------------------------------------------------

void
UITextBox::setText(const sf::String& t) {
  m_text = t;
  m_cursorPos = std::min(m_cursorPos, static_cast<int>(m_text.getSize()));
  m_onTextChanged(m_text);
}

void
UITextBox::setCursorPos(int pos) {
  m_cursorPos = std::clamp(pos, 0, static_cast<int>(m_text.getSize()));
}

// -----------------------------------------------------------------------------
// Text input (keyboard — called from onUpdate when focused)
// -----------------------------------------------------------------------------

void
UITextBox::handleTextInput() {
  if (!m_focused) return;
  if (!Keyboard::isStarted()) return;
  Keyboard& kb = Keyboard::instance();
  bool changed = false;

  if (kb.wasPressedThisFrame(Key::kBackspace)) {
    if (m_cursorPos > 0) {
      m_text.erase(m_cursorPos - 1);
      --m_cursorPos;
      changed = true;
    }
    return;
  }

  if (kb.wasPressedThisFrame(Key::kDelete)) {
    if (m_cursorPos < static_cast<int>(m_text.getSize())) {
      m_text.erase(m_cursorPos);
      changed = true;
    }
    return;
  }

  if (kb.wasPressedThisFrame(Key::kLeft)) {
    m_cursorPos = std::max(0, m_cursorPos - 1);
    return;
  }

  if (kb.wasPressedThisFrame(Key::kRight)) {
    m_cursorPos = std::min(static_cast<int>(m_text.getSize()), m_cursorPos + 1);
    return;
  }

  if (kb.wasPressedThisFrame(Key::kHome)) {
    m_cursorPos = 0;
    return;
  }

  if (kb.wasPressedThisFrame(Key::kEnd)) {
    m_cursorPos = static_cast<int>(m_text.getSize());
    return;
  }

  if (kb.wasPressedThisFrame(Key::kEnter)) {
    m_onConfirm();
    return;
  }

  // Tab is consumed by the canvas focus cycling — skip here.
  if (kb.wasPressedThisFrame(Key::kTab))
    return;

  if (changed)
    m_onTextChanged(m_text);
}

// -----------------------------------------------------------------------------
// Text input (canvas-routed — printable characters)
// -----------------------------------------------------------------------------

void
UITextBox::onTextInput(const Vector<char32_t>& chars) {
  if (chars.empty()) return;
  for (char32_t ch : chars) {
    if (ch >= 32 && ch < 0x10FFFF && ch != 127) {
      m_text.insert(m_cursorPos, sf::String(ch));
      ++m_cursorPos;
    }
  }
  m_onTextChanged(m_text);
}

// -----------------------------------------------------------------------------
// Scroll offset
// -----------------------------------------------------------------------------

float
UITextBox::textXOffset() const {
  if (!m_font || m_size.x <= 0.f) return m_padding;
  const float usableW = m_size.x - m_padding * 2.f;

  sf::Text txt(*m_font, m_text, m_fontSize);
  const float textW = txt.getLocalBounds().size.x;

  if (textW <= usableW) return m_padding;

  const float cursorX = m_padding + prefixWidth(*m_font, m_text, m_cursorPos, m_fontSize);
  const float rightEdge = cursorX + 4.f;

  float offset = m_textOffsetX;
  if (rightEdge > m_padding + usableW)
    offset = m_padding + usableW - rightEdge;
  else if (cursorX + offset < m_padding)
    offset = m_padding - cursorX;
  return offset;
}

// -----------------------------------------------------------------------------
// Update
// -----------------------------------------------------------------------------

void
UITextBox::onUpdate(float dt) {
  if (!m_focused) return;

  m_cursorTimer += dt;
  if (m_cursorTimer >= m_cursorBlinkRate) {
    m_cursorTimer -= m_cursorBlinkRate;
    m_cursorVisible = !m_cursorVisible;
  }

  handleTextInput();
  m_textOffsetX = textXOffset();
}

// -----------------------------------------------------------------------------
// Draw
// -----------------------------------------------------------------------------

void
UITextBox::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (m_size.x <= 0.f || m_size.y <= 0.f) return;

  sf::RectangleShape bg(m_size);
  bg.setFillColor(m_focused ? m_focusedFillColor : m_fillColor);
  bg.setOutlineColor(m_focused ? m_focusedOutlineColor : m_outlineColor);
  bg.setOutlineThickness(1.f);
  target.draw(bg, states);

  if (!m_font) return;

  const bool hasText = !m_text.isEmpty();
  const sf::String& display = hasText ? m_text : m_placeholder;
  const sf::Color dispColor = hasText ? m_textColor : m_placeholderColor;

  sf::Text txt(*m_font, display, m_fontSize);
  txt.setFillColor(dispColor);
  txt.setPosition({m_textOffsetX, (m_size.y - txt.getLocalBounds().size.y) * 0.5f});
  target.draw(txt, states);

  if (m_focused && m_cursorVisible) {
    const float cursorX = m_textOffsetX
      + prefixWidth(*m_font, m_text, m_cursorPos, m_fontSize);
    const float pad  = 3.f;
    const float cH   = txt.getLocalBounds().size.y - pad * 2.f;

    sf::RectangleShape cursor({2.f, std::max(cH, 0.f)});
    cursor.setFillColor(m_cursorColor);
    cursor.setPosition({cursorX, (m_size.y - cH) * 0.5f});
    target.draw(cursor, states);
  }
}

} // namespace sfmx
