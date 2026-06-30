#include "scene/SpriteComponent.h"

#include <cmath>

#include "scene/SceneNode.h"
#include "resource/Frame.h"

#include "assets/AssetManager.h"
#include "assets/TextureAsset.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"   // operator<< / >> for UUID

namespace sfmx
{

namespace {
/** @brief SpriteComponent blob layout version; bump on format changes. */
constexpr uint32 kSpriteComponentVersion = 2;  // v2: + sprite scale
} // namespace

SpriteComponent::SpriteComponent(SceneNode* owner)
  : ComponentT<SpriteComponent>(owner)
{}

sf::Sprite&
SpriteComponent::activeSprite()
{
  SFMX_ASSERT(m_sprite);
  return *m_sprite;
}

const sf::Sprite&
SpriteComponent::activeSprite() const
{
  SFMX_ASSERT(m_sprite);
  return *m_sprite;
}

void
SpriteComponent::setTexture(const sf::Texture& texture)
{
  m_sprite = MakeShared<sf::Sprite>(texture);
}

void
SpriteComponent::setTexture(SPtr<sf::Texture> texture)
{
  m_texture = texture;
  if (m_texture) {
    m_sprite = MakeShared<sf::Sprite>(*m_texture);
  }
}

void
SpriteComponent::setTextureAsset(SPtr<TextureAsset> asset)
{
  // If handed an asset that isn't decoded yet, bring it up through the
  // AssetManager by its UUID — the same resolution path as setTextureAssetId.
  // A single attempt (no delegation back to setTextureAssetId) keeps this from
  // recursing if load() ever hands back a non-null but still-unloaded asset.
  if (nullptr != asset && !asset->isLoaded() && AssetManager::isStarted()) {
    SPtr<TextureAsset> loaded =
        AssetManager::instance().load<TextureAsset>(asset->metadata().uuid);
    if (nullptr != loaded) {
      asset = loaded;
    }
  }

  m_textureAsset   = asset;
  m_textureAssetId = (nullptr != asset) ? asset->metadata().uuid : UUID::null();
  if (nullptr != asset && asset->isLoaded()) {
    m_sprite = MakeShared<sf::Sprite>(asset->texture());
  }
}

void
SpriteComponent::setTextureAssetId(const UUID& id)
{
  if (id != UUID::null() && AssetManager::isStarted()) {
    SPtr<TextureAsset> asset = AssetManager::instance().load<TextureAsset>(id);
    if (nullptr != asset) {
      setTextureAsset(asset);  // records m_textureAssetId from the asset (== id)
      return;
    }
  }
  // Couldn't resolve (no manager, null id, or not cataloged): keep the id so it
  // still re-serializes and can resolve later.
  m_textureAssetId = id;
}

const UUID&
SpriteComponent::getTextureAssetId() const
{
  return m_textureAssetId;
}

SPtr<TextureAsset>
SpriteComponent::getTextureAsset() const
{
  return m_textureAsset;
}

void
SpriteComponent::setRect(const sf::IntRect& rect)
{
  activeSprite().setTextureRect(rect);
}

const sf::IntRect&
SpriteComponent::getRect() const
{
  return activeSprite().getTextureRect();
}

void
SpriteComponent::setColor(const sf::Color& color)
{
  activeSprite().setColor(color);
}

sf::Color
SpriteComponent::getColor() const
{
  return activeSprite().getColor();
}

void
SpriteComponent::move(const sf::Vector2f& delta)
{
  activeSprite().move(delta);
}

void
SpriteComponent::setPosition(const sf::Vector2f& position)
{
  activeSprite().setPosition(position);
}

sf::Vector2f
SpriteComponent::getPosition() const
{
  return activeSprite().getPosition();
}

void
SpriteComponent::rotate(float deltaDegrees)
{
  activeSprite().rotate(sf::degrees(deltaDegrees));
}

void
SpriteComponent::rotate(const sf::Angle& angle)
{
  activeSprite().rotate(angle);
}

void
SpriteComponent::setRotation(const sf::Angle& angle)
{
  activeSprite().setRotation(angle);
}

void
SpriteComponent::setRotation(float degrees)
{
  activeSprite().setRotation(sf::degrees(degrees));
}

sf::Angle
SpriteComponent::getRotation() const
{
  return activeSprite().getRotation();
}

float
SpriteComponent::getRotationDegrees() const
{
  return activeSprite().getRotation().asDegrees();
}

void
SpriteComponent::scale(float delta)
{
  activeSprite().scale({delta, delta});
}

void
SpriteComponent::scale(const sf::Vector2f& delta)
{
  activeSprite().scale(delta);
}

void
SpriteComponent::setScale(float newScale)
{
  activeSprite().setScale({newScale, newScale});
}

void
SpriteComponent::setScale(const sf::Vector2f& newScale)
{
  activeSprite().setScale(newScale);
}

sf::Vector2f
SpriteComponent::getScale() const
{
  return activeSprite().getScale();
}

void
SpriteComponent::setOrigin(const sf::Vector2f origin)
{
  activeSprite().setOrigin(origin);
}

sf::Vector2f
SpriteComponent::getOrigin() const
{
  return activeSprite().getOrigin();
}

void
SpriteComponent::flipX(bool flipped)
{
  m_flipX = flipped;
}

void
SpriteComponent::flipY(bool flipped)
{
  m_flipY = flipped;
}

bool
SpriteComponent::isFlippedX() const
{
  return m_flipX;
}

bool
SpriteComponent::isFlippedY() const
{
  return m_flipY;
}

void
SpriteComponent::setFrame(const Frame& frame)
{
  if (frame.texture) {
    m_texture = frame.texture;
    m_sprite = MakeShared<sf::Sprite>(*m_texture, frame.framing);
  } else if (frame.framing.size.x > 0 && frame.framing.size.y > 0) {
    setRect(frame.framing);
  }

  setColor(frame.color);
  flipX(frame.flippedX);
  flipY(frame.flippedY);

  setScale(frame.scale);
  setPosition(frame.position);
  setRotation(frame.rotation);
}

const Frame
SpriteComponent::getAsFrame() const
{
  Frame f;
  const sf::Sprite& s = activeSprite();
  f.texture = m_texture;
  f.framing = s.getTextureRect();
  f.color = s.getColor();
  f.flippedX = m_flipX;
  f.flippedY = m_flipY;
  f.scale = s.getScale();
  f.position = s.getPosition();
  f.rotation = s.getRotation();
  return f;
}

sf::Vector2i
SpriteComponent::getPixelSize()
{
  return activeSprite().getTextureRect().size;
}

void
SpriteComponent::onUpdate(float /*deltaTime*/)
{

}

void
SpriteComponent::onDraw(sf::RenderTarget& target,
                        sf::RenderStates states) const
{
  if (m_sprite)
  {
    states.transform = m_owner->getWorldTransform();
    states.transform.scale({ m_flipX ? -1.f : 1.f, m_flipY ? -1.f : 1.f });
    target.draw(*m_sprite, states);
  }
}

// -----------------------------------------------------------------------------
// Serialization
// -----------------------------------------------------------------------------

void
SpriteComponent::onSerialize(DataStream& stream) const
{
  stream << kSpriteComponentVersion;
  stream << m_textureAssetId;                 // UUID; null for a raw / no texture
  stream << static_cast<uint8>(m_flipX ? 1 : 0);
  stream << static_cast<uint8>(m_flipY ? 1 : 0);

  const uint8 hasSprite = (nullptr != m_sprite) ? 1 : 0;
  stream << hasSprite;
  if (hasSprite) {
    const sf::IntRect rect = m_sprite->getTextureRect();
    stream << rect.position.x << rect.position.y << rect.size.x << rect.size.y;
    const sf::Color color = m_sprite->getColor();
    stream << color.r << color.g << color.b << color.a;
    const sf::Vector2f scale = m_sprite->getScale();
    stream << scale.x << scale.y;
  }
}

void
SpriteComponent::onDeserialize(DataStream& stream)
{
  uint32 version = 0;
  stream >> version;
  if (version != kSpriteComponentVersion) {
    return;  // unknown version: leave defaults rather than misread bytes
  }

  UUID id;
  stream >> id;

  uint8 flipX = 0;
  uint8 flipY = 0;
  uint8 hasSprite = 0;
  stream >> flipX >> flipY >> hasSprite;
  m_flipX = flipX != 0;
  m_flipY = flipY != 0;

  // Re-resolve the texture by UUID; this (re)creates m_sprite when the asset loads.
  setTextureAssetId(id);

  if (hasSprite) {
    // Always consume the sprite-state bytes, even if the texture didn't resolve.
    int32 rx = 0;
    int32 ry = 0;
    int32 rw = 0;
    int32 rh = 0;
    stream >> rx >> ry >> rw >> rh;
    uint8 cr = 0;
    uint8 cg = 0;
    uint8 cb = 0;
    uint8 ca = 255;
    stream >> cr >> cg >> cb >> ca;
    float sx = 1.f;
    float sy = 1.f;
    stream >> sx >> sy;
    if (nullptr != m_sprite) {
      m_sprite->setTextureRect(sf::IntRect({rx, ry}, {rw, rh}));
      m_sprite->setColor(sf::Color(cr, cg, cb, ca));
      m_sprite->setScale({sx, sy});
    }
  }
}

} // namespace sfmx
