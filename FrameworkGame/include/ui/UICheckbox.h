/************************************************************************/
/**
 * @file UICheckbox.h
 * @author Swampertor
 * @date 2026/06/10
 * @brief  Toggle checkbox with optional texture fallback.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include "scene/SceneNode.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

class TextureAsset;
class UICheckboxGroup;

class UICheckbox final : public UIWidgetT<UICheckbox, WidgetType::kCheckbox>,
                         public ComponentT<UICheckbox>
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
  using UIWidget::onPointerEnter;
  using UIWidget::onPointerExit;
  using UIWidget::onPointerClick;

  /** @brief  Standalone constructor (no SceneNode). */
  UICheckbox(sf::Vector2f size = {24.f, 24.f});
  /** @brief  Component constructor attached to a SceneNode. */
  UICheckbox(SceneNode* node, sf::Vector2f size = {24.f, 24.f});
  ~UICheckbox() override;

  /** @brief  Type UUID for serialization. */
  NODISCARD UUID getTypeId() const override;
  /** @brief  Serialize checked state, colors and texture asset ID. */
  void onSerialize(DataStream& stream) const override;
  /** @brief  Restore state written by onSerialize. */
  void onDeserialize(DataStream& stream) override;

  /** @brief  Whether the box is currently checked. */
  FORCEINLINE bool isChecked() const { return m_checked; }
  /** @brief  Set checked state with optional silent update. */
  void setChecked(bool checked, bool notify = true);

  /** @brief  Subscribe to checked-state changes (bool). */
  NODISCARD FORCEINLINE HEvent onValueChanged(Function<void(bool)> cb) const
    { return m_onValueChangedEvent.connect(std::move(cb)); }

  /** @brief  Override the normal box colour. */
  FORCEINLINE void setBoxColor(sf::Color color) { m_boxColor = color; }
  /** @brief  Override the check-mark colour. */
  FORCEINLINE void setCheckColor(sf::Color color) { m_checkColor = color; }
  /** @brief  Override the hover-highlight colour. */
  FORCEINLINE void setHoveredBoxColor(sf::Color color) { m_hoveredBoxColor = color; }
  /** @brief  Override the checked-state colour. */
  FORCEINLINE void setCheckedBoxColor(sf::Color color) { m_checkedBoxColor = color; }

  /** @brief  Current normal box colour. */
  NODISCARD FORCEINLINE sf::Color getBoxColor() const { return m_boxColor; }
  /** @brief  Current check-mark colour. */
  NODISCARD FORCEINLINE sf::Color getCheckColor() const { return m_checkColor; }
  /** @brief  Current hover-highlight colour. */
  NODISCARD FORCEINLINE sf::Color getHoveredBoxColor() const { return m_hoveredBoxColor; }
  /** @brief  Current checked-state colour. */
  NODISCARD FORCEINLINE sf::Color getCheckedBoxColor() const { return m_checkedBoxColor; }

  // -- Texture asset -----------------------------------------------------------

  /** @brief  Set a texture asset (replaces the procedural box + checkmark). */
  void setTextureAsset(SPtr<TextureAsset> asset);
  /** @brief  Set texture by asset UUID (resolved via AssetManager). */
  void setTextureAssetId(const UUID& id);
  /** @brief  UUID of the assigned texture asset. */
  NODISCARD const UUID& getTextureAssetId() const;
  /** @brief  The assigned texture asset, or nullptr. */
  NODISCARD SPtr<TextureAsset> getTextureAsset() const;
  /** @brief  True if a texture sprite is available for drawing. */
  NODISCARD FORCEINLINE bool hasTexture() const { return m_sprite != nullptr; }

  // -- Group ------------------------------------------------------------------

  /** @brief  Assign this checkbox to a group (or nullptr to leave). */
  void setGroup(UICheckboxGroup* group);
  /** @brief  Current group, or nullptr. */
  NODISCARD UICheckboxGroup* getGroup() const;

 private:
  friend class UICheckboxGroup;
  /** @brief  Hover highlight on enter. */
  void onPointerEnter(sf::Vector2f position) override;
  /** @brief  Remove hover highlight on exit. */
  void onPointerExit(sf::Vector2f position) override;
  /** @brief  Toggle checked state on click. */
  void onPointerClick(sf::Vector2f position) override;
  /** @brief  Draw the box + checkmark, or the texture sprite. */
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  bool m_checked = false;
  bool m_hovered = false;
  UICheckboxGroup* m_group = nullptr;

  mutable sf::RectangleShape m_box;
  mutable sf::VertexArray m_checkMark;
  mutable UniquePtr<sf::Sprite> m_sprite;
  SPtr<TextureAsset> m_textureAsset;
  UUID m_textureAssetId = UUID::null();

  sf::Color m_boxColor = sf::Color(200, 200, 200);
  sf::Color m_hoveredBoxColor = sf::Color(180, 180, 255);
  sf::Color m_checkedBoxColor = sf::Color(100, 200, 100);
  sf::Color m_checkColor = sf::Color::White;

  Event<void(bool)> mutable m_onValueChangedEvent;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UICheckbox)
