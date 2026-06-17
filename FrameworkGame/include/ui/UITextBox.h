/************************************************************************/
/**
 * @file UITextBox.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Single-line text input widget.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/System/String.hpp>

#include "scene/Component.h"
#include "ui/UIWidget.h"
#include "utils/EventSystem.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief A single-line text input box.
 *
 * Supports typing, backspace, delete, cursor movement with arrow keys,
 * Home/End, placeholder text, and a blinking cursor.  Must be focusable
 * and focused to receive keyboard input; clicking the box gives it focus.
 */
class UITextBox : public ComponentT<UITextBox>, public UIWidget
{
 public:
  explicit UITextBox(SceneNode* owner);

  void onUpdate(float dt) override;
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;
  void onTextInput(const Vector<char32_t>& chars) override;

  // Text

  /** @brief Set the text content */
  void setText(const sf::String& t);
  /** @brief Current text content */
  NODISCARD FORCEINLINE const sf::String& 
  getText() const { return m_text; }

  // Font

  FORCEINLINE void 
  setFont(SPtr<sf::Font> f)        { m_font = std::move(f); }
  FORCEINLINE void 
  setFontSize(unsigned s)          { m_fontSize = s; }

  // Colors

  FORCEINLINE void 
  setTextColor(sf::Color c)         { m_textColor = c; }
  FORCEINLINE void 
  setCursorColor(sf::Color c)       { m_cursorColor = c; }
  FORCEINLINE void 
  setFillColor(sf::Color c)         { m_fillColor = c; }
  FORCEINLINE void 
  setFocusedFillColor(sf::Color c)  { m_focusedFillColor = c; }
  FORCEINLINE void 
  setOutlineColor(sf::Color c)      { m_outlineColor = c; }
  FORCEINLINE void 
  setFocusedOutlineColor(sf::Color c) { m_focusedOutlineColor = c; }

  // Placeholder

  FORCEINLINE void 
  setPlaceholder(const sf::String& t) { m_placeholder = t; }
  NODISCARD FORCEINLINE const sf::String& 
  getPlaceholder() const { return m_placeholder; }

  // Events

  /** @brief Fired when text content changes */
  NODISCARD FORCEINLINE Event<void(const sf::String&)>& 
  onTextChanged() { return m_onTextChanged; }
  /** @brief Fired when Enter/Return is pressed */
  NODISCARD FORCEINLINE Event<void()>& 
  onConfirm()       { return m_onConfirm; }

  // Cursor

  void setCursorPos(int pos);
  NODISCARD FORCEINLINE int  
  getCursorPos() const { return m_cursorPos; }

 private:
  void handleTextInput();
  NODISCARD float textXOffset() const;

  sf::String   m_text;
  sf::String   m_placeholder;
  SPtr<sf::Font> m_font;
  unsigned     m_fontSize           = 16;
  sf::Color    m_textColor          = {220, 220, 220};
  sf::Color    m_cursorColor        = {200, 200, 200};
  sf::Color    m_fillColor          = {40, 40, 45};
  sf::Color    m_focusedFillColor   = {50, 55, 65};
  sf::Color    m_outlineColor       = {80, 80, 85};
  sf::Color    m_focusedOutlineColor = {100, 150, 220};
  sf::Color    m_placeholderColor   = {120, 120, 125};

  int    m_cursorPos       = 0;
  float  m_cursorTimer     = 0.f;
  bool   m_cursorVisible   = true;
  float  m_cursorBlinkRate = 0.53f;
  float  m_padding         = 6.f;
  float  m_textOffsetX     = 0.f;

  Event<void(const sf::String&)> m_onTextChanged;
  Event<void()>                  m_onConfirm;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UITextBox)
