/************************************************************************/
/**
 * @file UITextBox.h
 * @author Swampertor
 * @date 2026/06/10
 * @brief  Single-line text input box with placeholder, cursor, and text clipping.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include "scene/SceneNode.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

class UITextBox final : public UIWidgetT<UITextBox, WidgetType::kTextBox>,
                        public ComponentT<UITextBox>
{
 public:
  using UIWidget::isEnabled;
  using UIWidget::isVisible;
  using UIWidget::isInteractable;
  using UIWidget::setEnabled;
  using UIWidget::setVisible;
  using UIWidget::setInteractable;
  using UIWidget::setFocused;
  using UIWidget::getPosition;
  using UIWidget::setPosition;
  using UIWidget::getSize;
  using UIWidget::setSize;
  using UIWidget::getRect;
  using UIWidget::setRect;
  using UIWidget::getColor;
  using UIWidget::setColor;
  using UIWidget::containsPoint;
  using UIWidget::syncColliderToRect;
  using UIWidget::onPointerDown;
  using UIWidget::onSelect;
  using UIWidget::onDeselect;

  /** @brief  Standalone constructor (no SceneNode). */
  UITextBox(sf::Vector2f size = {200.f, 40.f});
  /** @brief  Component constructor attached to a SceneNode. */
  UITextBox(SceneNode* node, sf::Vector2f size = {200.f, 40.f});
  ~UITextBox() override = default;

  /** @brief  Type UUID for serialization. */
  NODISCARD UUID getTypeId() const override;
  /** @brief  Serialize text content, char size, colors and texture ID. */
  void onSerialize(DataStream& stream) const override;
  /** @brief  Restore state written by onSerialize. */
  void onDeserialize(DataStream& stream) override;

  /** @brief  Replace the displayed text. */
  FORCEINLINE void setText(StringView text) {
    m_textContent = String(text);
    syncText();
  }
  /** @brief  Current text content. */
  NODISCARD FORCEINLINE StringView getText() const {
    return m_textContent;
  }

  /** @brief  Assign a shared font (creates the sf::Text internally). */
  void setFont(SPtr<sf::Font> font);
  /** @brief  Currently assigned font. */
  NODISCARD FORCEINLINE SPtr<sf::Font> getFont() const { return m_font; }

  /** @brief  Set the font size in pixels. */
  FORCEINLINE void setCharacterSize(uint32 size) {
    m_charSize = size;
    if (m_text) { m_text->setCharacterSize(size); }
  }
  /** @brief  Current font size. */
  NODISCARD FORCEINLINE uint32 getCharacterSize() const { return m_charSize; }

  /** @brief  Colour of the typed text. */
  FORCEINLINE void setTextColor(sf::Color color) {
    if (m_text) { m_text->setFillColor(color); }
  }
  /** @brief  Current text colour. */
  NODISCARD FORCEINLINE sf::Color getTextColor() const {
    return m_text ? m_text->getFillColor() : sf::Color::White;
  }

  /** @brief  Background fill colour. */
  FORCEINLINE void setBackgroundColor(sf::Color color) { m_bgColor = color; }
  /** @brief  Border colour when focused. */
  FORCEINLINE void setFocusedBorderColor(sf::Color color) { m_focusedBorderColor = color; }
  /** @brief  Current background colour. */
  NODISCARD FORCEINLINE sf::Color getBackgroundColor() const { return m_bgColor; }
  /** @brief  Current focused-border colour. */
  NODISCARD FORCEINLINE sf::Color getFocusedBorderColor() const { return m_focusedBorderColor; }

  /** @brief  Move the text cursor to a character index. */
  FORCEINLINE void setCursorPosition(uint32 pos) { m_cursorPos = pos; }
  /** @brief  Current cursor character index. */
  NODISCARD FORCEINLINE uint32 getCursorPosition() const { return m_cursorPos; }

  /** @brief  Text shown when the input is empty. */
  FORCEINLINE void setPlaceholder(StringView text) { m_placeholder = String(text); }
  /** @brief  Current placeholder text. */
  NODISCARD FORCEINLINE const String& getPlaceholder() const { return m_placeholder; }
  /** @brief  Colour of the placeholder text. */
  FORCEINLINE void setPlaceholderColor(sf::Color color) { m_placeholderColor = color; }
  /** @brief  Current placeholder colour. */
  NODISCARD FORCEINLINE sf::Color getPlaceholderColor() const { return m_placeholderColor; }

  /** @brief  Returns true (skips keyboard navigation while focused). */
  bool isTextEditor() const override { return true; }

  /** @brief  Insert a Unicode code-point at the cursor. */
  void insertCharacter(uint32 unicode);
  /** @brief  Delete the character before the cursor (backspace). */
  void deleteCharacter();
  /** @brief  Delete the character after the cursor (delete key). */
  void deleteForward();

 private:
  /** @brief  Focus the textbox and move cursor to the click position. */
  void onPointerDown(sf::Vector2f position) override;
  /** @brief  Draw background, border, clipped text, and optional cursor. */
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  /** @brief  Push m_textContent into the sf::Text. */
  void syncText();
  /** @brief  Update cursor shape position from m_cursorPos. */
  void syncCursor();

  String m_textContent;
  String m_placeholder;
  mutable UniquePtr<sf::Text> m_text;
  mutable sf::Vector2f m_lastPos;
  mutable sf::Vector2f m_lastSize;
  mutable bool m_dirty = true;
  mutable sf::RectangleShape m_background;
  mutable sf::RectangleShape m_border;
  mutable sf::RectangleShape m_cursorShape;
  // TODO: Change this to FontAsset when it is implemented
  SPtr<sf::Font> m_font;
  uint32 m_cursorPos = 0;
  uint32 m_charSize = 20;

  sf::Color m_bgColor = sf::Color(30, 30, 30);
  sf::Color m_borderColor = sf::Color(80, 80, 80);
  sf::Color m_focusedBorderColor = sf::Color(100, 150, 255);
  sf::Color m_cursorColor = sf::Color::White;
  sf::Color m_textColor = sf::Color::White;
  sf::Color m_placeholderColor = sf::Color(120, 120, 120);
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UITextBox)
