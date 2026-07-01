#include "ui/UILabel.h"
#include "core/DataStream.h"

namespace sfmx
{

namespace {
/** @brief UILabel blob layout version; bump on format changes. */
constexpr uint32 kUILabelVersion = 1;
} // namespace

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

UUID UILabel::getTypeId() const {
  return TypeTraits<UILabel>::getTypeId();
}

void UILabel::setFont(SPtr<sf::Font> font) {
  m_font = font;
  if (m_font) {
    m_text = MakeUnique<sf::Text>(*m_font);
  } else {
    m_text.reset();
  }
}

void UILabel::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!isVisible() || !m_text) {
    return;
  }

  m_text->setPosition(getPosition());
  target.draw(*m_text, states);
}

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

  if (!m_font) {
    // Can't set text without a font; skip but still consume bytes.
    String text = stream.readString();
    uint32 charSize = 20;
    stream >> charSize;
    uint8 r = 255, g = 255, b = 255, a = 255;
    stream >> r >> g >> b >> a;
    return;
  }

  if (!m_text) {
    m_text = MakeUnique<sf::Text>(*m_font);
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
