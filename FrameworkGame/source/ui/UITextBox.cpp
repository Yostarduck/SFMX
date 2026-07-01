#include "ui/UITextBox.h"
#include "core/DataStream.h"

#include <SFML/Graphics/View.hpp>
#include <SFML/System/String.hpp>

namespace sfmx
{

UITextBox::UITextBox(sf::Vector2f size)
  : UIWidgetT<UITextBox, WidgetType::kTextBox>(),
    ComponentT<UITextBox>(nullptr) {
  setSize(size);
  syncColliderToRect();
}

UITextBox::UITextBox(SceneNode* node, sf::Vector2f size)
  : UIWidgetT<UITextBox, WidgetType::kTextBox>(),
    ComponentT<UITextBox>(node) {
  setSize(size);
  syncColliderToRect();
}

UITextBox::~UITextBox() = default;

UUID UITextBox::getTypeId() const {
  return TypeTraits<UITextBox>::getTypeId();
}

void UITextBox::setFont(SPtr<sf::Font> font) {
  m_font = font;
  if (m_font) {
    m_text = MakeUnique<sf::Text>(*m_font);
    m_text->setCharacterSize(m_charSize);
    syncText();
  } else {
    m_text.reset();
  }
}

void UITextBox::syncText() {
  if (m_text) {
    m_text->setString(sf::String(m_textContent));
  }
}

void UITextBox::insertCharacter(uint32 unicode) {
  sf::String str(m_textContent);
  str.insert(m_cursorPos, sf::String(static_cast<char32_t>(unicode)));
  m_textContent = str.toAnsiString();
  ++m_cursorPos;
  syncText();
}

void UITextBox::deleteCharacter() {
  if (m_cursorPos == 0) {
    return;
  }
  sf::String str(m_textContent);
  str.erase(m_cursorPos - 1);
  m_textContent = str.toAnsiString();
  --m_cursorPos;
  syncText();
}

void UITextBox::deleteForward() {
  sf::String str(m_textContent);
  if (m_cursorPos >= str.getSize()) {
    return;
  }
  str.erase(m_cursorPos);
  m_textContent = str.toAnsiString();
  syncText();
}

void UITextBox::onPointerDown(sf::Vector2f position) {
  UIWidget::onPointerDown(position);
  // Position cursor by click position (approximate: place at end for now).
  // Full character-index-from-position would need per-glyph advance queries.
  if (m_text) {
    m_cursorPos = static_cast<uint32>(m_textContent.size());
  }
}

void UITextBox::onDraw(sf::RenderTarget& target,
                        sf::RenderStates states) const {
  if (!UIWidget::s_canvasDrawing) return;
  if (!isVisible()) {
    return;
  }

  const sf::Vector2f pos = getPosition();
  const sf::Vector2f size = getSize();
  constexpr float borderThickness = 2.f;

  m_background.setSize(size);
  m_background.setPosition(pos);
  m_background.setFillColor(m_bgColor);
  target.draw(m_background, states);

  m_border.setSize(size);
  m_border.setPosition(pos);
  m_border.setFillColor(sf::Color::Transparent);
  m_border.setOutlineThickness(borderThickness);
  m_border.setOutlineColor(isFocused() ? m_focusedBorderColor : m_borderColor);
  target.draw(m_border, states);

  constexpr float textPadding = 6.f;
  const float innerRight = pos.x + size.x - textPadding;

  // Clip text to the textbox interior.
  const sf::View prevView = target.getView();
  const sf::Vector2u targetSize = target.getSize();
  sf::View clipView(sf::FloatRect(pos, size));
  clipView.setViewport(sf::FloatRect(
    {pos.x / targetSize.x, pos.y / targetSize.y},
    {size.x / targetSize.x, size.y / targetSize.y}
  ));
  target.setView(clipView);

  if (m_text) {
    if (m_textContent.empty() && !m_placeholder.empty()) {
      const sf::Color savedColor = m_text->getFillColor();
      m_text->setString(sf::String(m_placeholder));
      m_text->setFillColor(m_placeholderColor);
      m_text->setPosition({pos.x + textPadding, pos.y + textPadding});
      target.draw(*m_text, states);
      m_text->setFillColor(savedColor);
      m_text->setString(sf::String(m_textContent));
    } else if (!m_textContent.empty()) {
      m_text->setPosition({pos.x + textPadding, pos.y + textPadding});
      target.draw(*m_text, states);
    }

    if (isFocused()) {
      const float contentW = m_textContent.empty() ? 0.f
        : m_text->getGlobalBounds().size.x;
      const float charWidth = m_textContent.empty() ? 0.f
        : contentW / static_cast<float>(m_textContent.size());
      const float cursorX = pos.x + textPadding
        + charWidth * std::min(m_cursorPos,
            static_cast<uint32>(m_textContent.size()));
      const sf::Vector2f cursorSize = {
        2.f, static_cast<float>(m_charSize)
      };
      m_cursorShape.setSize(cursorSize);
      m_cursorShape.setPosition({std::min(cursorX, innerRight), pos.y + textPadding});
      m_cursorShape.setFillColor(m_cursorColor);
      target.draw(m_cursorShape, states);
    }
  }

  target.setView(prevView);
}

void UITextBox::onSerialize(DataStream& stream) const {
  constexpr uint32 kVersion = 1;
  stream << kVersion;

  uint8 flags = 0;
  if (isEnabled())       flags |= 1 << 0;
  if (isVisible())       flags |= 1 << 1;
  if (isInteractable())  flags |= 1 << 2;
  if (isFocused())       flags |= 1 << 3;
  if (isBlockingInput()) flags |= 1 << 4;
  stream << flags;

  const sf::FloatRect& r = getRect();
  stream << r.position.x << r.position.y << r.size.x << r.size.y;

  stream << getAnchorMin().x << getAnchorMin().y
         << getAnchorMax().x << getAnchorMax().y
         << getPivot().x     << getPivot().y;

  const sf::Color& c = getColor();
  stream << c.r << c.g << c.b << c.a;

  stream.writeString(m_textContent);
  stream << m_charSize;

  const sf::Color tc = m_text ? m_text->getFillColor() : sf::Color::White;
  stream << tc.r << tc.g << tc.b << tc.a;
  stream << m_bgColor.r << m_bgColor.g << m_bgColor.b << m_bgColor.a;
  stream << m_focusedBorderColor.r << m_focusedBorderColor.g
         << m_focusedBorderColor.b << m_focusedBorderColor.a;
}

void UITextBox::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != 1) {
    return;
  }

  uint8 flags = 0;
  stream >> flags;
  setEnabled((flags & (1 << 0)) != 0);
  setVisible((flags & (1 << 1)) != 0);
  setInteractable((flags & (1 << 2)) != 0);
  setFocused((flags & (1 << 3)) != 0);
  setBlocksInput((flags & (1 << 4)) != 0);

  sf::FloatRect r;
  stream >> r.position.x >> r.position.y >> r.size.x >> r.size.y;
  setRect(r);

  sf::Vector2f val;
  stream >> val.x >> val.y; setAnchorMin(val);
  stream >> val.x >> val.y; setAnchorMax(val);
  stream >> val.x >> val.y; setPivot(val);

  uint8 cr, cg, cb, ca;
  stream >> cr >> cg >> cb >> ca;
  setColor(sf::Color(cr, cg, cb, ca));

  m_textContent = stream.readString();
  stream >> m_charSize;

  uint8 tr, tg, tb, ta;
  stream >> tr >> tg >> tb >> ta;
  m_textColor = sf::Color(tr, tg, tb, ta);
  stream >> m_bgColor.r >> m_bgColor.g >> m_bgColor.b >> m_bgColor.a;
  stream >> m_focusedBorderColor.r >> m_focusedBorderColor.g
         >> m_focusedBorderColor.b >> m_focusedBorderColor.a;

  if (m_font && !m_text) {
    m_text = MakeUnique<sf::Text>(*m_font);
    m_text->setCharacterSize(m_charSize);
  }
  if (m_text) {
    syncText();
    m_text->setFillColor(m_textColor);
  }
  m_cursorPos = static_cast<uint32>(m_textContent.size());
}

} // namespace sfmx
