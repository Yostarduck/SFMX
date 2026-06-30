#include "ui/UIImage.h"

#include "assets/AssetManager.h"
#include "assets/TextureAsset.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"

namespace sfmx
{

UIImage::UIImage(sf::Vector2f size)
  : UIWidgetT<UIImage, WidgetType::kImage>(),
    ComponentT<UIImage>(nullptr) {
  setSize(size);
}

UIImage::UIImage(SceneNode* node, sf::Vector2f size)
  : UIWidgetT<UIImage, WidgetType::kImage>(),
    ComponentT<UIImage>(node) {
  setSize(size);
}

UIImage::~UIImage() = default;

UUID UIImage::getTypeId() const {
  return TypeTraits<UIImage>::getTypeId();
}

void UIImage::setTextureAsset(SPtr<TextureAsset> asset) {
  if (nullptr != asset && !asset->isLoaded() && AssetManager::isStarted()) {
    SPtr<TextureAsset> loaded =
        AssetManager::instance().load<TextureAsset>(asset->metadata().uuid);
    if (nullptr != loaded) {
      asset = loaded;
    }
  }

  m_textureAsset = asset;
  m_textureAssetId = (nullptr != asset) ? asset->metadata().uuid : UUID::null();
  if (nullptr != asset && asset->isLoaded()) {
    m_sprite = MakeUnique<sf::Sprite>(asset->texture());
  } else {
    m_sprite.reset();
  }
}

void UIImage::setTextureAssetId(const UUID& id) {
  if (id != UUID::null() && AssetManager::isStarted()) {
    SPtr<TextureAsset> asset = AssetManager::instance().load<TextureAsset>(id);
    if (nullptr != asset) {
      setTextureAsset(asset);
      return;
    }
  }
  m_textureAssetId = id;
  m_sprite.reset();
}

const UUID& UIImage::getTextureAssetId() const {
  return m_textureAssetId;
}

SPtr<TextureAsset> UIImage::getTextureAsset() const {
  return m_textureAsset;
}

void UIImage::setTextureRect(const sf::IntRect& rect) {
  if (m_sprite) {
    m_sprite->setTextureRect(rect);
  }
}

sf::IntRect UIImage::getTextureRect() const {
  return m_sprite ? m_sprite->getTextureRect() : sf::IntRect();
}

void UIImage::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!isVisible() || !m_sprite) {
    return;
  }

  const sf::Vector2f pos = getPosition();
  const sf::Vector2f size = getSize();

  m_sprite->setPosition(pos);
  m_sprite->setScale({
    (m_flipX ? -1.f : 1.f) * (size.x / m_sprite->getLocalBounds().size.x),
    (m_flipY ? -1.f : 1.f) * (size.y / m_sprite->getLocalBounds().size.y)
  });

  target.draw(*m_sprite, states);
}

void UIImage::onSerialize(DataStream& stream) const {
  constexpr uint32 kVersion = 1;
  stream << kVersion;
  stream << m_textureAssetId;
  stream << static_cast<uint8>(m_flipX ? 1 : 0);
  stream << static_cast<uint8>(m_flipY ? 1 : 0);

  stream << static_cast<uint8>(m_sprite ? 1 : 0);
  if (m_sprite) {
    const sf::IntRect rect = m_sprite->getTextureRect();
    stream << rect.position.x << rect.position.y << rect.size.x << rect.size.y;
    const sf::Color color = m_sprite->getColor();
    stream << color.r << color.g << color.b << color.a;
  }
}

void UIImage::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != 1) {
    return;
  }

  UUID id;
  stream >> id;
  m_textureAssetId = id;

  uint8 flipX = 0, flipY = 0, hasSprite = 0;
  stream >> flipX >> flipY >> hasSprite;
  m_flipX = flipX != 0;
  m_flipY = flipY != 0;

  if (hasSprite) {
    int32 rx = 0, ry = 0, rw = 0, rh = 0;
    stream >> rx >> ry >> rw >> rh;
    uint8 cr = 255, cg = 255, cb = 255, ca = 255;
    stream >> cr >> cg >> cb >> ca;

    // Resolve texture by UUID; this creates m_sprite on success.
    setTextureAssetId(id);

    if (m_sprite) {
      m_sprite->setTextureRect(sf::IntRect({rx, ry}, {rw, rh}));
      m_sprite->setColor(sf::Color(cr, cg, cb, ca));
    }
  }
}

} // namespace sfmx
