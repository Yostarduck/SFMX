/************************************************************************/
/**
 * @file UISlider.h
 * @author Swampertor
 * @date 2026/06/10
 * @brief  Horizontal slider with value range, stepping, and optional thumb texture.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include "scene/SceneNode.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

class TextureAsset;

class UISlider final : public UIWidgetT<UISlider, WidgetType::kSlider>,
                       public ComponentT<UISlider>
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
  using UIWidget::onPointerUp;

  /** @brief  Standalone constructor (no SceneNode). */
  UISlider(sf::Vector2f size = {200.f, 20.f});
  /** @brief  Component constructor attached to a SceneNode. */
  UISlider(SceneNode* node, sf::Vector2f size = {200.f, 20.f});
  ~UISlider() override = default;

  /** @brief  Type UUID for serialization. */
  NODISCARD UUID getTypeId() const override;
  /** @brief  Serialize value, range, step, thumb size, colors and texture ID. */
  void onSerialize(DataStream& stream) const override;
  /** @brief  Restore state written by onSerialize. */
  void onDeserialize(DataStream& stream) override;

  /** @brief  Current slider value. */
  NODISCARD FORCEINLINE float getValue() const { return m_value; }
  /** @brief  Set value (clamped to range, snapped to step). */
  void setValue(float value, bool notify = true);
  /** @brief  Lower bound of the value range. */
  NODISCARD FORCEINLINE float getMinValue() const { return m_minValue; }
  /** @brief  Upper bound of the value range. */
  NODISCARD FORCEINLINE float getMaxValue() const { return m_maxValue; }
  /** @brief  Set both min and max (re-clamps current value). */
  void setRange(float min, float max);

  /** @brief  Current value remapped to [0, 1]. */
  NODISCARD FORCEINLINE float getNormalizedValue() const {
    const float range = m_maxValue - m_minValue;
    return (range > 0.f) ? (m_value - m_minValue) / range : 0.f;
  }

  /** @brief  Snap increment (0 = continuous). */
  FORCEINLINE void setStepValue(float step) { m_stepValue = std::max(0.f, step); }
  /** @brief  Current snap increment. */
  NODISCARD FORCEINLINE float getStepValue() const { return m_stepValue; }

  /** @brief  Colour of the background track. */
  void setTrackColor(sf::Color color);
  /** @brief  Colour of the filled portion. */
  void setFillColor(sf::Color color);
  /** @brief  Colour of the draggable thumb. */
  void setThumbColor(sf::Color color);
  /** @brief  Diameter of the thumb in pixels. */
  void setThumbSize(float size);

  /** @brief  Current track colour. */
  NODISCARD FORCEINLINE sf::Color getTrackColor() const { return m_trackColor; }
  /** @brief  Current fill colour. */
  NODISCARD FORCEINLINE sf::Color getFillColor() const { return m_fillColor; }
  /** @brief  Current thumb colour. */
  NODISCARD FORCEINLINE sf::Color getThumbColor() const { return m_thumbColor; }
  /** @brief  Current thumb diameter. */
  NODISCARD FORCEINLINE float getThumbSize() const { return m_thumbSize; }

  // -- Texture asset for the thumb --------------------------------------------

  /** @brief  Set a texture asset for the thumb (replaces the circle). */
  void setThumbTextureAsset(SPtr<TextureAsset> asset);
  /** @brief  Set thumb texture by asset UUID. */
  void setThumbTextureAssetId(const UUID& id);
  /** @brief  UUID of the assigned thumb texture. */
  NODISCARD const UUID& getThumbTextureAssetId() const;
  /** @brief  The assigned thumb texture asset, or nullptr. */
  NODISCARD SPtr<TextureAsset> getThumbTextureAsset() const;
  /** @brief  True if a thumb sprite is available. */
  NODISCARD FORCEINLINE bool hasThumbTexture() const { return m_thumbSprite != nullptr; }

  /** @brief  Subscribe to value-change events (float). */
  NODISCARD FORCEINLINE HEvent onValueChanged(Function<void(float)> cb) const
    { return m_onValueChangedEvent.connect(std::move(cb)); }

 private:
  /** @brief  Begin drag, snap thumb to click position. */
  void triggerPointerDown(sf::Vector2f position) override;
  /** @brief  End drag. */
  void triggerPointerUp(sf::Vector2f position) override;
  /** @brief  Per-frame drag update (reads pointer state). */
  void onUpdate(float deltaTime) override;
  /** @brief  Draw track, fill, and thumb (circle or sprite). */
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  /** @brief  Convert local-space X to a clamped/stepped value. */
  void updateValueFromLocalX(float localX);
  /** @brief  Local-space X of the thumb centre (based on current value). */
  NODISCARD float getThumbCenterX() const;

  mutable float m_value = 0.5f;
  float m_minValue = 0.f;
  float m_maxValue = 1.f;
  float m_stepValue = 0.f;
  float m_thumbSize = 16.f;
  mutable bool m_dragging = false;

  mutable sf::RectangleShape m_track;
  mutable sf::RectangleShape m_fill;
  mutable sf::CircleShape m_thumb;
  mutable UniquePtr<sf::Sprite> m_thumbSprite;
  mutable bool m_visualDirty = true;
  SPtr<TextureAsset> m_thumbTextureAsset;
  UUID m_thumbTextureAssetId = UUID::null();

  sf::Color m_trackColor = sf::Color(60, 60, 60);
  sf::Color m_fillColor = sf::Color(100, 150, 255);
  sf::Color m_thumbColor = sf::Color(200, 200, 200);

  Event<void(float)> mutable m_onValueChangedEvent;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UISlider)
