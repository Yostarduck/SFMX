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
constexpr uint32 kUILabelVersion = 2; ///< Blob version; bump on format changes.
} // anonymous namespace

// -- Constructors ------------------------------------------------------------

UILabel::UILabel(sf::Vector2f size)
  : UIWidgetT<UILabel, WidgetType::kLabel>() {
  m_blocksInput = false; // Labels are non-interactive.
  setSize(size);
}

UILabel::~UILabel()  {
  m_text.reset();
  m_fontAsset.reset();
  m_text.release();
}

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
    // Apply backing state set while no font was available (e.g. right after
    // a UI document load resolves the font UUID).
    m_text->setString(m_textContent);
    m_text->setCharacterSize(m_charSize);
    m_text->setFillColor(m_textColor);
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
  if (!isVisible() || !m_text) {
    return;
  }

  m_text->setPosition(getPosition());
  target.draw(*m_text, states);
}

// -- Serialization -----------------------------------------------------------

void UILabel::onSerialize(DataStream& stream) const {
  stream << kUILabelVersion;
  serializeBase(stream);

  // Text/char size/colour come from the font-independent backing store, so a
  // label serializes identically whether or not its font has resolved.
  const sf::U8String utf8 = m_textContent.toUtf8();
  stream.writeString(String(reinterpret_cast<const char*>(utf8.data()),
                            utf8.size()));
  stream << m_charSize;
  stream << m_textColor.r << m_textColor.g << m_textColor.b << m_textColor.a;
}

void UILabel::onDeserialize(DataStream& stream) {
  // TODO: When the FontAsset is made, add here the UUID
  uint32 version = 0;
  stream >> version;
  if (version != kUILabelVersion) {
    return;
  }
  deserializeBase(stream);

  const String text = stream.readString();
  m_textContent = sf::String::fromUtf8(text.begin(), text.end());
  stream >> m_charSize;

  uint8 r = 255, g = 255, b = 255, a = 255;
  stream >> r >> g >> b >> a;
  m_textColor = sf::Color(r, g, b, a);

  // Apply now if the font is already resolved; otherwise setFontAsset()
  // pushes the backing store when the font UUID gets loaded later.
  if (m_text) {
    m_text->setString(m_textContent);
    m_text->setCharacterSize(m_charSize);
    m_text->setFillColor(m_textColor);
  }
}

} // namespace sfmx
