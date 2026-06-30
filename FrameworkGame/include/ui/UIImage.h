#pragma once

#include <SFML/Graphics/Sprite.hpp>

#include "scene/SceneNode.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

class TextureAsset;

/**
 * @brief A UI widget that displays a texture (asset-backed or raw).
 *
 * Follows the same dual-mode pattern as UIButton: can live standalone in a
 * Canvas or on a SceneNode via ComponentT<UIImage>.
 *
 * Texture asset management mirrors SpriteComponent:
 *   - setTextureAsset() stores a keep-alive SPtr<TextureAsset> and records its UUID.
 *   - setTextureAssetId() resolves the UUID through AssetManager at runtime.
 *   - Serialization round-trips the UUID so the texture re-resolves on load.
 */
class UIImage final : public UIWidgetT<UIImage, WidgetType::kImage>, public ComponentT<UIImage>
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

  UIImage(sf::Vector2f size = {200.f, 200.f});

  UIImage(SceneNode* node,
          sf::Vector2f size = {200.f, 200.f});

  ~UIImage() override;

  NODISCARD UUID getTypeId() const override;

  // -- Texture asset (SpriteComponent-style) ----------------------------------

  void setTextureAsset(SPtr<TextureAsset> asset);
  void setTextureAssetId(const UUID& id);

  NODISCARD const UUID& getTextureAssetId() const;
  NODISCARD SPtr<TextureAsset> getTextureAsset() const;

  void setTextureRect(const sf::IntRect& rect);
  NODISCARD sf::IntRect getTextureRect() const;
  NODISCARD FORCEINLINE bool hasTexture() const { return m_sprite != nullptr; }

  FORCEINLINE void setFlipX(bool flip) { m_flipX = flip; }
  FORCEINLINE void setFlipY(bool flip) { m_flipY = flip; }
  NODISCARD FORCEINLINE bool isFlippedX() const { return m_flipX; }
  NODISCARD FORCEINLINE bool isFlippedY() const { return m_flipY; }

  // -- Serialization ---------------------------------------------------------

  void onSerialize(DataStream& stream) const override;
  void onDeserialize(DataStream& stream) override;

 private:
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  mutable UniquePtr<sf::Sprite> m_sprite;
  SPtr<TextureAsset> m_textureAsset;
  UUID m_textureAssetId = UUID::null();
  bool m_flipX = false;
  bool m_flipY = false;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::UIImage)
