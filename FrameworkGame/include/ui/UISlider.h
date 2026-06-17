/************************************************************************/
/**
 * @file UISlider.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Draggable slider widget for numeric input.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/Color.hpp>

#include "scene/Component.h"
#include "ui/UIWidget.h"
#include "utils/EventSystem.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief Horizontal slider with a draggable thumb.
 *
 * Maps a pointer position along the widget width to a float value
 * in [min, max].  Fires onValueChanged while the thumb is dragged.
 */
class UISlider : public ComponentT<UISlider>, public UIWidget
{
 public:
  explicit UISlider(SceneNode* owner);

  void onUpdate(float dt) override;
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  // Range

  void  setRange(float min, float max);
  FORCEINLINE float 
  getMin() const { return m_min; }
  FORCEINLINE float 
  getMax() const { return m_max; }

  // Value

  void  setValue(float v);
  NODISCARD FORCEINLINE float getValue() const { return m_value; }

  // Visual

  FORCEINLINE void 
  setTrackColor(sf::Color c)      { m_trackColor = c; }
  FORCEINLINE void 
  setFillColor(sf::Color c)       { m_fillColor = c; }
  FORCEINLINE void 
  setThumbColor(sf::Color c)      { m_thumbColor = c; }
  FORCEINLINE void 
  setThumbHoverColor(sf::Color c) { m_thumbHoverColor = c; }
  FORCEINLINE void 
  setThumbFocusColor(sf::Color c) { m_thumbFocusColor = c; }
  FORCEINLINE void 
  setThumbSize(float s)           { m_thumbSize = s; }
  FORCEINLINE void 
  setFocusOutlineColor(sf::Color c) { m_focusOutlineColor = c; }

  // Event

  /** @brief Called every frame the value changes (during drag) */
  NODISCARD FORCEINLINE Event<void(float)>& 
  onValueChanged() { return m_onValueChanged; }

 private:
  NODISCARD float valueFromPointer() const;
  NODISCARD sf::Vector2f thumbCenter() const;
  NODISCARD sf::FloatRect trackRect() const;
  NODISCARD sf::FloatRect thumbRect() const;

  float  m_min        = 0.f;
  float  m_max        = 1.f;
  float  m_value      = 0.5f;
  float  m_thumbSize  = 16.f;
  float  m_trackPad   = 8.f;   // horizontal padding for the track

  sf::Color m_trackColor         = {50, 50, 55};
  sf::Color m_fillColor          = {70, 100, 180};
  sf::Color m_thumbColor         = {200, 200, 210};
  sf::Color m_thumbHoverColor    = {220, 220, 230};
  sf::Color m_thumbFocusColor    = {210, 210, 220};
  sf::Color m_focusOutlineColor  = {100, 150, 220};

  Event<void(float)> m_onValueChanged;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UISlider)
