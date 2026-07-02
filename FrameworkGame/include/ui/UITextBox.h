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

  UITextBox(sf::Vector2f size = {200.f, 40.f});
  UITextBox(SceneNode* node, sf::Vector2f size = {200.f, 40.f});
  ~UITextBox() override;

  NODISCARD UUID getTypeId() const override;
  void onSerialize(DataStream& stream) const override;
  void onDeserialize(DataStream& stream) override;

  FORCEINLINE void setText(StringView text) {
    m_textContent = String(text);
    syncText();
  }
  NODISCARD FORCEINLINE String getText() const {
    return m_textContent;
  }

  void setFont(SPtr<sf::Font> font);
  NODISCARD FORCEINLINE SPtr<sf::Font> getFont() const { return m_font; }

  FORCEINLINE void setCharacterSize(uint32 size) {
    m_charSize = size;
    if (m_text) { m_text->setCharacterSize(size); }
  }
  NODISCARD FORCEINLINE uint32 getCharacterSize() const { return m_charSize; }

  FORCEINLINE void setTextColor(sf::Color color) {
    if (m_text) { m_text->setFillColor(color); }
  }
  NODISCARD FORCEINLINE sf::Color getTextColor() const {
    return m_text ? m_text->getFillColor() : sf::Color::White;
  }

  FORCEINLINE void setBackgroundColor(sf::Color color) { m_bgColor = color; }
  FORCEINLINE void setFocusedBorderColor(sf::Color color) { m_focusedBorderColor = color; }
  NODISCARD FORCEINLINE sf::Color getBackgroundColor() const { return m_bgColor; }
  NODISCARD FORCEINLINE sf::Color getFocusedBorderColor() const { return m_focusedBorderColor; }

  FORCEINLINE void setCursorPosition(uint32 pos) { m_cursorPos = pos; }
  NODISCARD FORCEINLINE uint32 getCursorPosition() const { return m_cursorPos; }

  FORCEINLINE void setPlaceholder(StringView text) { m_placeholder = String(text); }
  NODISCARD FORCEINLINE const String& getPlaceholder() const { return m_placeholder; }
  FORCEINLINE void setPlaceholderColor(sf::Color color) { m_placeholderColor = color; }
  NODISCARD FORCEINLINE sf::Color getPlaceholderColor() const { return m_placeholderColor; }

  bool isTextEditor() const override { return true; }

  void insertCharacter(uint32 unicode);
  void deleteCharacter();
  void deleteForward();

 private:
  void onPointerDown(sf::Vector2f position) override;
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  void syncText();
  void syncCursor();

  String m_textContent;
  String m_placeholder;
  mutable UniquePtr<sf::Text> m_text;
  mutable sf::RectangleShape m_background;
  mutable sf::RectangleShape m_border;
  mutable sf::RectangleShape m_cursorShape;
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
