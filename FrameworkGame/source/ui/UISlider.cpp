#include "ui/UISlider.h"
#include "ui/UIEventSystem.h"
#include "assets/AssetManager.h"
#include "assets/TextureAsset.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"
#include "utils/Arithmetic.h"

#include <cmath>

namespace sfmx
{

UISlider::UISlider(sf::Vector2f size)
  : UIWidgetT<UISlider, WidgetType::kSlider>(),
    ComponentT<UISlider>(nullptr) {
  setSize(size);
  syncColliderToRect();
}

UISlider::UISlider(SceneNode* node, sf::Vector2f size)
  : UIWidgetT<UISlider, WidgetType::kSlider>(),
    ComponentT<UISlider>(node) {
  setSize(size);
  syncColliderToRect();
}

UUID UISlider::getTypeId() const {
  return TypeTraits<UISlider>::getTypeId();
}

void UISlider::setValue(float value, bool notify) {
  value = std::max(m_minValue, std::min(m_maxValue, value));
  if (m_stepValue > 0.f) {
    value = m_minValue + std::round((value - m_minValue) / m_stepValue) * m_stepValue;
    value = std::max(m_minValue, std::min(m_maxValue, value));
  }
  if (m_value != value) {
    m_value = value;
    m_visualDirty = true;
    if (notify) {
      m_onValueChangedEvent(value);
    }
  }
}

void UISlider::setRange(float min, float max) {
  m_minValue = min;
  m_maxValue = max;
  m_visualDirty = true;
  setValue(m_value);
}

void UISlider::setTrackColor(sf::Color color) { 
  m_trackColor = color; 
  m_visualDirty = true; 
}

void UISlider::setFillColor(sf::Color color) { 
  m_fillColor = color; 
  m_visualDirty = true; 
}

void UISlider::setThumbColor(sf::Color color) { 
  m_thumbColor = color; 
  m_visualDirty = true; 
}

void UISlider::setThumbSize(float size) { 
  m_thumbSize = size; 
  m_visualDirty = true; 
}

// -- Texture asset for the thumb -----------------------------------------------

void UISlider::setThumbTextureAsset(SPtr<TextureAsset> asset) {
  if (nullptr != asset && !asset->isLoaded() && AssetManager::isStarted()) {
    SPtr<TextureAsset> loaded =
        AssetManager::instance().load<TextureAsset>(asset->metadata().uuid);
    if (nullptr != loaded) {
      asset = loaded;
    }
  }

  m_thumbTextureAsset = asset;
  m_thumbTextureAssetId = (nullptr != asset) ? asset->metadata().uuid : UUID::null();
  if (nullptr != asset && asset->isLoaded()) {
    m_thumbSprite = MakeUnique<sf::Sprite>(asset->texture());
  } 
  else {
    m_thumbSprite.reset();
  }
}

void UISlider::setThumbTextureAssetId(const UUID& id) {
  if (id != UUID::null() && AssetManager::isStarted()) {
    SPtr<TextureAsset> asset = AssetManager::instance().load<TextureAsset>(id);
    if (nullptr != asset) {
      setThumbTextureAsset(asset);
      return;
    }
  }
  m_thumbTextureAssetId = id;
  m_thumbSprite.reset();
}

const UUID& UISlider::getThumbTextureAssetId() const {
  return m_thumbTextureAssetId;
}

SPtr<TextureAsset> UISlider::getThumbTextureAsset() const {
  return m_thumbTextureAsset;
}

// ------------------------------------------------------------------------------

void UISlider::updateValueFromLocalX(float localX) {
  const float w = getSize().x;
  if (w <= 0.f) {
    return;
  }
  const float t = std::max(0.f, std::min(1.f, localX / w));
  setValue(lerp::number(m_minValue, m_maxValue, t));
}

float UISlider::getThumbCenterX() const {
  const float w = getSize().x;
  if (w <= 0.f) {
    return 0.f;
  }
  const float t = (m_value - m_minValue) / (m_maxValue - m_minValue);
  return t * w;
}

void UISlider::onPointerDown(sf::Vector2f position) {
  UIWidget::onPointerDown(position);
  updateValueFromLocalX(position.x);
  m_dragging = true;
}

void UISlider::onPointerUp(sf::Vector2f position) {
  m_dragging = false;
  UIWidget::onPointerUp(position);
}

void UISlider::onUpdate(float deltaTime) {
  SFMX_PARAMETER_UNUSED(deltaTime);
  if (!m_dragging || !UIEventSystem::isStarted()) {
    return;
  }

  const auto& ptr = UIEventSystem::instance().getPointerState();
  if (ptr.buttonDown) {
    const sf::Vector2f localPos = toLocalSpace(ptr.canvasPos);
    const float w = getSize().x;
    if (w > 0.f) {
      const float t = std::max(0.f, std::min(1.f, localPos.x / w));
      setValue(lerp::number(m_minValue, m_maxValue, t));
    }
  } 
  else {
    m_dragging = false;
  }
}

void UISlider::onDraw(sf::RenderTarget& target,
                       sf::RenderStates states) const {
  if (!UIWidget::s_canvasDrawing) return;
  if (!isVisible()) return;

  const sf::Vector2f pos = getPosition();
  const sf::Vector2f size = getSize();
  const float trackHeight = size.y * 0.4f;
  const float trackY = pos.y + (size.y - trackHeight) * 0.5f;
  const float thumbCX = pos.x + getThumbCenterX();

  if (m_visualDirty) {
    m_track.setSize({size.x, trackHeight});
    m_track.setPosition({pos.x, trackY});
    m_track.setFillColor(m_trackColor);

    const float fillWidth = thumbCX - pos.x;
    if (fillWidth > 0.f) {
      m_fill.setSize({fillWidth, trackHeight});
      m_fill.setPosition({pos.x, trackY});
      m_fill.setFillColor(m_fillColor);
    }

    if (m_thumbSprite) {
      const sf::FloatRect tb = m_thumbSprite->getLocalBounds();
      if (tb.size.x > 0.f && tb.size.y > 0.f) {
        const float half = m_thumbSize * 0.5f;
        m_thumbSprite->setPosition({thumbCX - half, pos.y + (size.y - m_thumbSize) * 0.5f});
        m_thumbSprite->setScale({m_thumbSize / tb.size.x, m_thumbSize / tb.size.y});
      }
    } else {
      m_thumb.setRadius(m_thumbSize * 0.5f);
      m_thumb.setOrigin({m_thumbSize * 0.5f, m_thumbSize * 0.5f});
      m_thumb.setPosition({thumbCX, pos.y + size.y * 0.5f});
      m_thumb.setFillColor(m_thumbColor);
    }

    m_visualDirty = false;
  }

  target.draw(m_track, states);
  const float fillWidth = thumbCX - pos.x;
  if (fillWidth > 0.f) target.draw(m_fill, states);

  if (m_thumbSprite) {
    target.draw(*m_thumbSprite, states);
  } else {
    target.draw(m_thumb, states);
  }
}

void UISlider::onSerialize(DataStream& stream) const {
  constexpr uint32 kVersion = 2;
  stream << kVersion;

  uint8 flags = 0;
  if (isEnabled())       flags |= 1 << 0;
  if (isVisible())       flags |= 1 << 1;
  if (isInteractable())  flags |= 1 << 2;
  if (isFocused())       flags |= 1 << 3;
  if (isBlockingInput()) flags |= 1 << 4;
  stream << flags;

  const sf::FloatRect& r = getRect();
  stream << r.position.x << r.position.y << r.size.x << r.size.y;

  stream << getAnchorMin().x << getAnchorMin().y
         << getAnchorMax().x << getAnchorMax().y
         << getPivot().x     << getPivot().y;

  const sf::Color& c = getColor();
  stream << c.r << c.g << c.b << c.a;

  stream << m_value << m_minValue << m_maxValue << m_stepValue;
  stream << m_thumbSize;

  stream << m_trackColor.r << m_trackColor.g << m_trackColor.b << m_trackColor.a;
  stream << m_fillColor.r << m_fillColor.g << m_fillColor.b << m_fillColor.a;
  stream << m_thumbColor.r << m_thumbColor.g << m_thumbColor.b << m_thumbColor.a;
  stream << m_thumbTextureAssetId;
}

void UISlider::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version < 1 || version > 2) {
    return;
  }

  uint8 flags = 0;
  stream >> flags;
  setEnabled((flags & (1 << 0)) != 0);
  setVisible((flags & (1 << 1)) != 0);
  setInteractable((flags & (1 << 2)) != 0);
  setFocused((flags & (1 << 3)) != 0);
  setBlocksInput((flags & (1 << 4)) != 0);

  sf::FloatRect r;
  stream >> r.position.x >> r.position.y >> r.size.x >> r.size.y;
  setRect(r);

  sf::Vector2f val;
  stream >> val.x >> val.y; setAnchorMin(val);
  stream >> val.x >> val.y; setAnchorMax(val);
  stream >> val.x >> val.y; setPivot(val);

  uint8 cr, cg, cb, ca;
  stream >> cr >> cg >> cb >> ca;
  setColor(sf::Color(cr, cg, cb, ca));

  stream >> m_value >> m_minValue >> m_maxValue;
  if (version >= 2) {
    stream >> m_stepValue;
  }
  stream >> m_thumbSize;
  setValue(m_value);

  uint8 tr, tg, tb, ta;
  stream >> tr >> tg >> tb >> ta; m_trackColor = sf::Color(tr, tg, tb, ta);
  stream >> tr >> tg >> tb >> ta; m_fillColor = sf::Color(tr, tg, tb, ta);
  stream >> tr >> tg >> tb >> ta; m_thumbColor = sf::Color(tr, tg, tb, ta);
  if (version >= 2) {
    UUID id;
    stream >> id;
    setThumbTextureAssetId(id);
  }
}

} // namespace sfmx
