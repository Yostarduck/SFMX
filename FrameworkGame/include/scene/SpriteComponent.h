#pragma once

#include "core/platform/Prerequisites.h"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Color.hpp>
#include "scene/Component.h"
#include "utils/UUID.h"

namespace sfmx {
class Frame;
class TextureAsset;

class SpriteComponent : public ComponentT<SpriteComponent> {

public:
  explicit SpriteComponent(SceneNode* owner);

  void setTexture(const sf::Texture& txt);
  void setTexture(SPtr<sf::Texture> texture);

  /** @brief Bind the sprite to a TextureAsset, keeping it alive and recording its UUID
   *         (the serializable handle). The asset's sf::Texture outlives the sprite. */
  void setTextureAsset(SPtr<TextureAsset> asset);
  /** @brief Record the texture asset UUID and resolve it via AssetManager when available;
   *         if it can't be resolved the id is still kept so it re-serializes. */
  void setTextureAssetId(const UUID& id);
  /** @brief UUID of the referenced TextureAsset, or UUID::null() for a raw/no texture. */
  NODISCARD const UUID& getTextureAssetId() const;
  /** @brief The kept-alive TextureAsset, or nullptr if the texture isn't asset-backed. */
  NODISCARD SPtr<TextureAsset> getTextureAsset() const;

  void setFrame(const Frame& f);
  NODISCARD const Frame getAsFrame() const;

  void setRect(const sf::IntRect& rect);

  NODISCARD const sf::IntRect& getRect() const;

  void setColor(const sf::Color& color);

  NODISCARD sf::Color getColor() const;

  void move(const sf::Vector2f& delta);
  void setPosition(const sf::Vector2f& position);

  NODISCARD sf::Vector2f getPosition() const;

  /** @brief Rotates the view by an angle in degrees */
  void rotate(float deltaDegrees);
  /** @brief Rotates the view by an sf::Angle */
  void rotate(const sf::Angle& angle);
  /** @brief Sets the rotation from an sf::Angle */
  void setRotation(const sf::Angle& angle);
  /** @brief Sets the rotation in degrees */
  void setRotation(float degrees);
  /** @brief Returns the current rotation angle */
  NODISCARD sf::Angle getRotation() const;
  /** @brief Returns the current rotation in degrees */
  NODISCARD float getRotationDegrees() const;

  void scale(float delta);
  void scale(const sf::Vector2f& delta);
  void setScale(float newScale);
  void setScale(const sf::Vector2f& newScale);
  NODISCARD sf::Vector2f getScale() const;

  void setOrigin(const sf::Vector2f newOrigin);

  NODISCARD sf::Vector2f getOrigin() const;

  NODISCARD sf::Vector2i getPixelSize();

  void flipX(bool flipped);
  void flipY(bool flipped);

  NODISCARD bool isFlippedX() const;
  NODISCARD bool isFlippedY() const;

  /** @brief Drives auto position from the node's world transform when m_followNode is true */
  void onUpdate(float deltaTime) override;

  void onDraw(sf::RenderTarget& target,
              sf::RenderStates states) const override;

  /** @brief Serializes the texture asset UUID, flips, color and texture rect. */
  void onSerialize(DataStream& stream) const override;
  /** @brief Restores the state written by @ref onSerialize (re-resolves the texture). */
  void onDeserialize(DataStream& stream) override;

  NODISCARD sf::Sprite& activeSprite();
  NODISCARD const sf::Sprite& activeSprite() const;

private:

  SPtr<sf::Sprite>     m_sprite;
  SPtr<sf::Texture>    m_texture;
  SPtr<TextureAsset>   m_textureAsset;            //!< keep-alive for an asset-backed texture
  UUID                 m_textureAssetId = UUID::null();
  bool                 m_flipX      = false;
  bool                 m_flipY      = false;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::SpriteComponent)
