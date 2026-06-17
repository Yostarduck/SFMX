/************************************************************************/
/**
 * @file UICheckbox.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Toggleable checkbox widget.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>

#include "scene/Component.h"
#include "ui/UIWidget.h"
#include "utils/EventSystem.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief A two-state toggle box with optional label.
 *
 * Fires onToggle() when the value changes.  The visual is a filled
 * rectangle with a check mark drawn when checked.
 */
class UICheckbox : public ComponentT<UICheckbox>, public UIWidget
{
 public:
  explicit UICheckbox(SceneNode* owner);

  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  // State

  /** @brief Set the checked state */
  void setChecked(bool v);
  /** @brief Whether the box is currently checked */
  NODISCARD bool isChecked() const { return m_checked; }

  // Label

  FORCEINLINE void 
  setText(const String& t)    { m_text = t; }
  FORCEINLINE const String& 
  getText() const             { return m_text; }
  FORCEINLINE void 
  setFont(SPtr<sf::Font> f)   { m_font = std::move(f); }
  FORCEINLINE void 
  setFontSize(unsigned s)     { m_fontSize = s; }
  FORCEINLINE void 
  setTextColor(sf::Color c)   { m_textColor = c; }

  // Colors

  FORCEINLINE void 
  setBoxColor(sf::Color c)          { m_boxColor = c; }
  FORCEINLINE void 
  setCheckColor(sf::Color c)        { m_checkColor = c; }
  FORCEINLINE void 
  setHoverBoxColor(sf::Color c)     { m_hoverBoxColor = c; }
  FORCEINLINE void 
  setPressBoxColor(sf::Color c)     { m_pressBoxColor = c; }
  FORCEINLINE void 
  setCheckedBoxColor(sf::Color c)   { m_checkedBoxColor = c; }
  FORCEINLINE void 
  setFocusBoxColor(sf::Color c)     { m_focusBoxColor = c; }
  FORCEINLINE void 
  setFocusOutlineColor(sf::Color c) { m_focusOutlineColor = c; }

  // Events

  /** @brief Fired when the checked state changes */
  FORCEINLINE Event<void(bool)>& 
  onToggle()         { return m_onToggle; }

 private:
  NODISCARD sf::Color 
  currentBoxColor() const;

  String          m_text;
  SPtr<sf::Font>  m_font;
  unsigned        m_fontSize     = 14;
  sf::Color       m_textColor         = {220, 220, 220};
  sf::Color       m_boxColor          = {60, 60, 65};
  sf::Color       m_checkColor        = {80, 220, 80};
  sf::Color       m_hoverBoxColor     = {110, 150, 230};
  sf::Color       m_pressBoxColor     = {80, 120, 210};
  sf::Color       m_checkedBoxColor   = {60, 130, 220};
  sf::Color       m_focusBoxColor     = {80, 85, 95};
  sf::Color       m_focusOutlineColor = {100, 150, 220};
  bool            m_checked          = false;
  float           m_boxSize          = 20.f;

  Event<void(bool)> m_onToggle;
  HEvent            m_clickConn;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UICheckbox)
