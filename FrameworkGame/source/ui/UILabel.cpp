/************************************************************************/
/**
 * @file UILabel.cpp
 * @author Swampertor
 * @date 2026/06/10
 * @brief  Non-interactive text label implementation.
 */
/************************************************************************/
#include "ui/UILabel.h"
#include "core/DataStream.h"

#include "assets/AssetManager.h"
#include "assets/FontAsset.h"

namespace sfmx
{

namespace {
constexpr uint32 kUILabelVersion = 1; ///< Blob version; bump on format changes.
} // anonymous namespace

// -- Constructors ------------------------------------------------------------

UILabel::UILabel(sf::Vector2f size)
  : UIWidgetT<UILabel, WidgetType::kLabel>(),
    ComponentT<UILabel>(nullptr) {
  setSize(size);
}

UILabel::UILabel(SceneNode* node, sf::Vector2f size)
  : UIWidgetT<UILabel, WidgetType::kLabel>(),
    ComponentT<UILabel>(node) {
  setSize(size);
}

UILabel::~UILabel() = default;

// -- Type --------------------------------------------------------------------

UUID UILabel::getTypeId() const {
  return TypeTraits<UILabel>::getTypeId();
}

// -- Font asset --------------------------------------------------------------

void UILabel::setFontAsset(SPtr<FontAsset> asset) {
  if (nullptr != asset && !asset->isLoaded() && AssetManager::isStarted()) {
    SPtr<FontAsset> loaded =
        AssetManager::instance().load<FontAsset>(asset->metadata().uuid);
    if (nullptr != loaded) {
      asset = loaded;
    }
  }

  m_fontAsset = asset;
  m_fontAssetId = (nullptr != asset) ? asset->metadata().uuid : UUID::null();
  if (nullptr != asset && asset->isLoaded()) {
    m_text = MakeUnique<sf::Text>(asset->font());
  } 
  else {
    m_text.reset();
  }
}

void UILabel::setFontAssetId(const UUID& id) {
  if (id != UUID::null() && AssetManager::isStarted()) {
    SPtr<FontAsset> asset = AssetManager::instance().load<FontAsset>(id);
    if (nullptr != asset) {
      setFontAsset(asset);
      return;
    }
  }
  m_fontAssetId = id;
}

const UUID& UILabel::getFontAssetId() const {
  return m_fontAssetId;
}

SPtr<FontAsset> UILabel::getFontAsset() const {
  return m_fontAsset;
}

// -- Drawing -----------------------------------------------------------------

void UILabel::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!UIWidget::s_canvasDrawing) return;
  if (!isVisible() || !m_text) {
    return;
  }

  m_text->setPosition(getPosition());
  target.draw(*m_text, states);
}

// -- Serialization -----------------------------------------------------------

void UILabel::onSerialize(DataStream& stream) const {
  stream << kUILabelVersion;
  stream.writeString(m_text ? m_text->getString().toAnsiString() : String());
  stream << static_cast<uint32>(m_text ? m_text->getCharacterSize() : 20);
  const sf::Color c = m_text ? m_text->getFillColor() : sf::Color::White;
  stream << c.r << c.g << c.b << c.a;
}

void UILabel::onDeserialize(DataStream& stream) {
  // TODO: When the FontAsset is made, add here the UUID
  uint32 version = 0;
  stream >> version;
  if (version != kUILabelVersion) {
    return;
  }

  if (!m_fontAsset) {
    // Can't set text without a font; skip but still consume bytes.
    String text = stream.readString();
    uint32 charSize = 20;
    stream >> charSize;
    uint8 r = 255, g = 255, b = 255, a = 255;
    stream >> r >> g >> b >> a;
    return;
  }

  if (!m_text) {
    m_text = MakeUnique<sf::Text>(m_fontAsset->font());
  }

  m_text->setString(stream.readString());

  uint32 charSize = 20;
  stream >> charSize;
  m_text->setCharacterSize(charSize);

  uint8 r = 255, g = 255, b = 255, a = 255;
  stream >> r >> g >> b >> a;
  m_text->setFillColor(sf::Color(r, g, b, a));
}

} // namespace sfmx
