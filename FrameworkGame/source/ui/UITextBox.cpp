#include "ui/UITextBox.h"
#include "ui/UIManager.h"
#include "core/DataStream.h"

#include <SFML/Graphics/View.hpp>
#include <SFML/System/String.hpp>

#include <algorithm>
#include <cmath>

#include "assets/AssetManager.h"
#include "assets/FontAsset.h"


namespace sfmx
{

namespace
{

/** @brief Horizontal padding between the box edge and the text/caret origin. */
constexpr float kTextPadding = 6.f;

} // namespace

UITextBox::UITextBox(sf::Vector2f size)
  : UIWidgetT<UITextBox, WidgetType::kTextBox>() {
  setSize(size);
}

UUID UITextBox::getTypeId() const {
  return TypeTraits<UITextBox>::getTypeId();
}

void UITextBox::setFontAsset(SPtr<FontAsset> asset) {
  if (nullptr != asset && !asset->isLoaded() && AssetManager::isStarted()) {
    SPtr<FontAsset> loaded =
        AssetManager::instance().load<FontAsset>(asset->metadata().uuid);
    if (nullptr != loaded) {
      asset = loaded;
    }
  }

  m_fontAsset = asset;
  m_fontAssetId = (nullptr != asset) ? asset->metadata().uuid : UUID::null();
  if (nullptr != asset && asset->isLoaded()) {
    m_text = MakeUnique<sf::Text>(asset->font());
    m_text->setCharacterSize(m_charSize);
    syncText();
  } 
  else {
    m_text.reset();
  }
}

void UITextBox::setFontAssetId(const UUID& id) {
  if (id != UUID::null() && AssetManager::isStarted()) {
    SPtr<FontAsset> asset = AssetManager::instance().load<FontAsset>(id);
    if (nullptr != asset) {
      setFontAsset(asset);
      return;
    }
  }
  m_fontAssetId = id;
}

const UUID& UITextBox::getFontAssetId() const {
  return m_fontAssetId;
}

SPtr<FontAsset> UITextBox::getFontAsset() const {
  return m_fontAsset;
}

void UITextBox::syncText() {
  if (m_text) {
    m_text->setString(m_textContent);
  }
}

void UITextBox::insertCharacter(uint32 unicode) {
  m_textContent.insert(m_cursorPos, sf::String(static_cast<char32_t>(unicode)));
  ++m_cursorPos;
  syncText();
}

void UITextBox::deleteCharacter() {
  if (m_cursorPos == 0) {
    return;
  }
  m_textContent.erase(m_cursorPos - 1);
  --m_cursorPos;
  syncText();
}

void UITextBox::deleteForward() {
  if (m_cursorPos >= m_textContent.getSize()) {
    return;
  }
  m_textContent.erase(m_cursorPos);
  syncText();
}

void UITextBox::triggerPointerDown(sf::Vector2f position) {
  UIWidget::triggerPointerDown(position);
  // Place the caret from the click: `position` is widget-local, the text
  // origin sits at (kTextPadding, kTextPadding), and findCharacterPos gives
  // each glyph origin in text-local space — pick the nearest boundary
  // (replaces the old "cursor at end" TODO and the average-char-width draw).
  if (!m_text) {
    return;
  }
  const std::size_t len = m_textContent.getSize();
  const float clickX = position.x - kTextPadding;

  float bestDist = std::fabs(clickX);  // distance to boundary 0
  std::size_t best = 0;
  for (std::size_t i = 1; i <= len; ++i) {
    const float glyphX = m_text->findCharacterPos(i).x;
    const float dist = std::fabs(glyphX - clickX);
    if (dist < bestDist) {
      bestDist = dist;
      best = i;
    }
  }
  m_cursorPos = static_cast<uint32>(best);
}

void UITextBox::triggerCancel() {
  // Subscribers see the cancel while we're still the selection, then dropping
  // selection unfocuses the box (setSelected drives setFocused/triggerDeselect)
  // — keyboard users are never trapped in the editor.
  UIWidget::triggerCancel();
  if (UIManager* manager = getManager()) {
    manager->setSelected(nullptr);
  }
}

void UITextBox::onDraw(sf::RenderTarget& target,
                        sf::RenderStates states) const {
  if (!isVisible()) {
    return;
  }

  const sf::Vector2f pos = getPosition();
  const sf::Vector2f size = getSize();

  // Rebuild background / border geometry only when position or size changes.
  if (m_dirty || pos != m_lastPos || size != m_lastSize) {
    m_lastPos = pos;
    m_lastSize = size;
    m_dirty = false;

    constexpr float bt = 2.f;
    m_background.setSize(size);
    m_background.setPosition(pos);
    m_border.setSize(size);
    m_border.setPosition(pos);
    m_border.setFillColor(sf::Color::Transparent);
    m_border.setOutlineThickness(bt);
  }
  m_background.setFillColor(m_bgColor);
  target.draw(m_background, states);
  m_border.setOutlineColor(isFocused() ? m_focusedBorderColor : m_borderColor);
  target.draw(m_border, states);

  const float innerRight = pos.x + size.x - kTextPadding;

  // Clip text to the textbox interior without overriding the parent view.
  const sf::View prevView = target.getView();
  const sf::Vector2u targetSize = target.getSize();
  sf::View clipView(prevView);
  {
    const sf::Vector2f screenPos = states.transform.transformPoint(pos);
    const sf::Vector2f screenSize =
      states.transform.transformPoint(pos + size) - screenPos;
    if (targetSize.x > 0 && targetSize.y > 0) {
      sf::FloatRect scissor(
        {screenPos.x / static_cast<float>(targetSize.x),
         screenPos.y / static_cast<float>(targetSize.y)},
        {screenSize.x / static_cast<float>(targetSize.x),
         screenSize.y / static_cast<float>(targetSize.y)});
      scissor.position.x = std::clamp(scissor.position.x, 0.f, 1.f);
      scissor.position.y = std::clamp(scissor.position.y, 0.f, 1.f);
      scissor.size.x = std::clamp(scissor.size.x, 0.f, 1.f - scissor.position.x);
      scissor.size.y = std::clamp(scissor.size.y, 0.f, 1.f - scissor.position.y);
      clipView.setScissor(scissor);
    }
  }
  target.setView(clipView);

  if (m_text) {
    if (m_textContent.isEmpty() && !m_placeholder.empty()) {
      const sf::Color savedColor = m_text->getFillColor();
      m_text->setString(
        sf::String::fromUtf8(m_placeholder.begin(), m_placeholder.end()));
      m_text->setFillColor(m_placeholderColor);
      m_text->setPosition({pos.x + kTextPadding, pos.y + kTextPadding});
      target.draw(*m_text, states);
      m_text->setFillColor(savedColor);
      m_text->setString(m_textContent);  // restore: caret math below runs on content
    } else if (!m_textContent.isEmpty()) {
      m_text->setPosition({pos.x + kTextPadding, pos.y + kTextPadding});
      target.draw(*m_text, states);
    }

    if (isFocused()) {
      // Exact glyph origin for the caret — no average-character-width guessing.
      const std::size_t caretIndex =
        std::min<std::size_t>(m_cursorPos, m_textContent.getSize());
      const float glyphX = m_text->findCharacterPos(caretIndex).x;
      m_cursorShape.setSize({2.f, static_cast<float>(m_charSize)});
      m_cursorShape.setPosition(
        {std::min(pos.x + kTextPadding + glyphX, innerRight),
         pos.y + kTextPadding});
      m_cursorShape.setFillColor(m_cursorColor);
      target.draw(m_cursorShape, states);
    }
  }

  target.setView(prevView);
}

void UITextBox::onSerialize(DataStream& stream) const {
  // Version 2: shared base state moved into UIWidget::serializeBase.
  constexpr uint32 kVersion = 2;
  stream << kVersion;
  serializeBase(stream);

  const sf::U8String utf8 = m_textContent.toUtf8();  // U8String → std::string
  stream.writeString(
    String(reinterpret_cast<const char*>(utf8.data()), utf8.size()));
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
  if (version != 2) {
    return;
  }
  deserializeBase(stream);

  const String utf8 = stream.readString();
  m_textContent = sf::String::fromUtf8(utf8.begin(), utf8.end());
  stream >> m_charSize;

  uint8 tr, tg, tb, ta;
  stream >> tr >> tg >> tb >> ta;
  m_textColor = sf::Color(tr, tg, tb, ta);
  stream >> m_bgColor.r >> m_bgColor.g >> m_bgColor.b >> m_bgColor.a;
  stream >> m_focusedBorderColor.r >> m_focusedBorderColor.g
         >> m_focusedBorderColor.b >> m_focusedBorderColor.a;

  if (m_fontAsset && !m_text) {
    m_text = MakeUnique<sf::Text>(m_fontAsset->font());
    m_text->setCharacterSize(m_charSize);
  }
  if (m_text) {
    syncText();
    m_text->setFillColor(m_textColor);
  }
  m_cursorPos = static_cast<uint32>(m_textContent.getSize());
}

} // namespace sfmx
