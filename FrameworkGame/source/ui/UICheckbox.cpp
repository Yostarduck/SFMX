#include "ui/UICheckbox.h"
#include "ui/UICheckboxGroup.h"
#include "assets/AssetManager.h"
#include "assets/TextureAsset.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"

#include <cmath>

namespace sfmx
{

UICheckbox::UICheckbox(sf::Vector2f size)
  : UIWidgetT<UICheckbox, WidgetType::kCheckbox>(),
    ComponentT<UICheckbox>(nullptr) {
  setSize(size);
  syncColliderToRect();
}

UICheckbox::UICheckbox(SceneNode* node, sf::Vector2f size)
  : UIWidgetT<UICheckbox, WidgetType::kCheckbox>(),
    ComponentT<UICheckbox>(node) {
  setSize(size);
  syncColliderToRect();
}

UICheckbox::~UICheckbox()
{
  if (m_group) {
    m_group->removeCheckbox(this);
  }
}

UUID UICheckbox::getTypeId() const {
  return TypeTraits<UICheckbox>::getTypeId();
}

void UICheckbox::setChecked(bool checked, bool notify) {
  if (m_checked != checked) {
    if (checked && m_group && m_group->isExclusive()) {
      m_group->onCheckboxChecked(this);
    }
    m_checked = checked;
    if (notify) {
      m_onValueChangedEvent(checked);
    }
  }
}

// -- Group -------------------------------------------------------------------

void UICheckbox::setGroup(UICheckboxGroup* group) {
  if (m_group == group) { 
    return; 
  }
  if (m_group) {
    m_group->removeCheckbox(this);
  }
  m_group = group;
  if (m_group) {
    m_group->m_checkboxes.push_back(this);
  }
}

UICheckboxGroup* UICheckbox::getGroup() const {
  return m_group;
}

// -- Texture asset ---------------------------------------------------------------

void UICheckbox::setTextureAsset(SPtr<TextureAsset> asset) {
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
  } 
  else {
    m_sprite.reset();
  }
}

void UICheckbox::setTextureAssetId(const UUID& id) {
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

const UUID& UICheckbox::getTextureAssetId() const {
  return m_textureAssetId;
}

SPtr<TextureAsset> UICheckbox::getTextureAsset() const {
  return m_textureAsset;
}

// --------------------------------------------------------------------------------

void UICheckbox::onPointerEnter(sf::Vector2f position) {
  m_hovered = true;
  UIWidget::onPointerEnter(position);
}

void UICheckbox::onPointerExit(sf::Vector2f position) {
  m_hovered = false;
  UIWidget::onPointerExit(position);
}

void UICheckbox::onPointerClick(sf::Vector2f position) {
  if (m_group && m_group->isExclusive() && m_checked) {
    return;
  }
  setChecked(!m_checked);
  UIWidget::onPointerClick(position);
}

void UICheckbox::onDraw(sf::RenderTarget& target,
                         sf::RenderStates states) const {
  if (!UIWidget::s_canvasDrawing) return;
  if (!isVisible()) {
    return;
  }

  const sf::Vector2f pos = getPosition();
  const sf::Vector2f size = getSize();

  if (m_sprite) {
    m_sprite->setPosition(pos);
    const sf::FloatRect sb = m_sprite->getLocalBounds();
    if (sb.size.x > 0.f && sb.size.y > 0.f) {
      m_sprite->setScale({size.x / sb.size.x, size.y / sb.size.y});
    }
    target.draw(*m_sprite, states);
    return;
  }

  if (m_checked) {
    m_box.setFillColor(m_checkedBoxColor);
  } 
  else if (m_hovered) {
    m_box.setFillColor(m_hoveredBoxColor);
  } 
  else {
    m_box.setFillColor(m_boxColor);
  }
  m_box.setSize(size);
  m_box.setPosition(pos);
  target.draw(m_box, states);

  if (m_checked) {
    constexpr float thickness = 3.f;
    const float pad = size.x * 0.2f;
    const float areaLeft = pos.x + pad;
    const float areaTop = pos.y + pad;
    const float areaSize = size.x - pad * 2.f;

    const sf::Vector2f start{areaLeft + areaSize * 0.15f, areaTop + areaSize * 0.6f};
    const sf::Vector2f corner{areaLeft + areaSize * 0.4f, areaTop + areaSize * 0.75f};
    const sf::Vector2f end{areaLeft + areaSize * 0.85f, areaTop + areaSize * 0.25f};

    m_checkMark.setPrimitiveType(sf::PrimitiveType::LineStrip);
    m_checkMark.resize(3);
    m_checkMark[0] = sf::Vertex(start, m_checkColor);
    m_checkMark[1] = sf::Vertex(corner, m_checkColor);
    m_checkMark[2] = sf::Vertex(end, m_checkColor);
    target.draw(m_checkMark, states);
  }
}

void UICheckbox::onSerialize(DataStream& stream) const {
  constexpr uint32 kVersion = 2;
  stream << kVersion;

  uint8 flags = 0;
  if (isEnabled())       flags |= 1 << 0;
  if (isVisible())       flags |= 1 << 1;
  if (isInteractable())  flags |= 1 << 2;
  if (isFocused())       flags |= 1 << 3;
  if (isBlockingInput()) flags |= 1 << 4;
  if (m_checked)         flags |= 1 << 5;
  stream << flags;

  const sf::FloatRect& r = getRect();
  stream << r.position.x << r.position.y << r.size.x << r.size.y;

  stream << getAnchorMin().x << getAnchorMin().y
         << getAnchorMax().x << getAnchorMax().y
         << getPivot().x     << getPivot().y;

  const sf::Color& c = getColor();
  stream << c.r << c.g << c.b << c.a;

  stream << m_boxColor.r << m_boxColor.g << m_boxColor.b << m_boxColor.a;
  stream << m_hoveredBoxColor.r << m_hoveredBoxColor.g
         << m_hoveredBoxColor.b << m_hoveredBoxColor.a;
  stream << m_checkedBoxColor.r << m_checkedBoxColor.g
         << m_checkedBoxColor.b << m_checkedBoxColor.a;
  stream << m_checkColor.r << m_checkColor.g << m_checkColor.b << m_checkColor.a;
  stream << m_textureAssetId;
}

void UICheckbox::onDeserialize(DataStream& stream) {
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
  m_checked = (flags & (1 << 5)) != 0;

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

  stream >> m_boxColor.r >> m_boxColor.g >> m_boxColor.b >> m_boxColor.a;
  stream >> m_hoveredBoxColor.r >> m_hoveredBoxColor.g
         >> m_hoveredBoxColor.b >> m_hoveredBoxColor.a;
  stream >> m_checkedBoxColor.r >> m_checkedBoxColor.g
         >> m_checkedBoxColor.b >> m_checkedBoxColor.a;
  stream >> m_checkColor.r >> m_checkColor.g >> m_checkColor.b >> m_checkColor.a;
  if (version >= 2) {
    UUID id;
    stream >> id;
    setTextureAssetId(id);
  }
}

} // namespace sfmx
