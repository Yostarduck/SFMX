#include "ui/UIButton.h"
#include "core/DataStream.h"

#include <SFML/Graphics/RectangleShape.hpp>

namespace sfmx
{

UIButton::UIButton(sf::Vector2f size)
  : UIWidgetT<UIButton, WidgetType::kButton>(),
    ComponentT<UIButton>(nullptr) {
  setSize(size);
  syncColliderToRect();
}

UIButton::UIButton(SceneNode* node, sf::Vector2f size)
  : UIWidgetT<UIButton, WidgetType::kButton>(),
    ComponentT<UIButton>(node) {
  setSize(size);
  syncColliderToRect();
}

UIButton::~UIButton() = default;

// -- Type --------------------------------------------------------------------

UUID UIButton::getTypeId() const {
  return TypeTraits<UIButton>::getTypeId();
}

// -- Pointer events ----------------------------------------------------------

void UIButton::onPointerEnter(sf::Vector2f position) {
  m_visualState = VisualState::kHovered;
  UIWidget::onPointerEnter(position);
}

void UIButton::onPointerExit(sf::Vector2f position) {
  m_visualState = VisualState::kNormal;
  UIWidget::onPointerExit(position);
}

void UIButton::onPointerDown(sf::Vector2f position) {
  m_visualState = VisualState::kPressed;
  UIWidget::onPointerDown(position);
}

void UIButton::onPointerUp(sf::Vector2f position) {
  m_visualState = VisualState::kHovered;
  UIWidget::onPointerUp(position);
}

// -- Drawing -----------------------------------------------------------------

void UIButton::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!isVisible()) {
    return;
  }

  sf::RectangleShape shape(getSize());
  shape.setPosition(getPosition());
  shape.setFillColor(resolveColor());
  target.draw(shape, states);
}

sf::Color UIButton::resolveColor() const {
  if (!isEnabled()) {
    return m_disabledColor;
  }

  // Priority: Pressed > Hovered > Focused > Normal
  switch (m_visualState) {
  case VisualState::kPressed:  return m_pressedColor;
  case VisualState::kHovered:  return m_hoveredColor;
  case VisualState::kFocused:
  case VisualState::kDisabled:
  case VisualState::kNormal:   break;
  }

  if (isFocused()) {
    return m_focusedColor;
  }

  return m_normalColor;
}

// -- Serialization ------------------------------------------------------------

void
UIButton::onSerialize(DataStream& stream) const {
  // Version
  constexpr uint32 kVersion = 1;
  stream << kVersion;

  // Widget state
  stream.writeString(getName());

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

  // Collider
  const ICollider* col = getCollider();
  if (col) {
    stream << static_cast<uint8>(col->getType());
    switch (col->getType()) {
      case ColliderType::kCircle: {
        const auto* c = static_cast<const CircleCollider*>(col);
        stream << c->getCenter().x << c->getCenter().y << c->getRadius();
        break;
      }
      case ColliderType::kAABB: {
        const auto* a = static_cast<const AABBCollider*>(col);
        stream << a->getCenter().x << a->getCenter().y
               << a->getHalfSize().x << a->getHalfSize().y;
        break;
      }
      case ColliderType::kOBB: {
        const auto* o = static_cast<const OBBCollider*>(col);
        stream << o->getCenter().x << o->getCenter().y
               << o->getHalfSize().x << o->getHalfSize().y;
        break;
      }
      case ColliderType::kPoint: {
        const auto* p = static_cast<const PointCollider*>(col);
        stream << p->getPoint().x << p->getPoint().y;
        break;
      }
      case ColliderType::kLine: {
        const auto* l = static_cast<const LineCollider*>(col);
        stream << l->getStart().x << l->getStart().y
               << l->getEnd().x   << l->getEnd().y;
        break;
      }
    }
  } else {
    stream << static_cast<uint8>(0xFF);
  }

  // Button colour overrides
  stream << m_normalColor.r   << m_normalColor.g
         << m_normalColor.b   << m_normalColor.a;
  stream << m_hoveredColor.r  << m_hoveredColor.g
         << m_hoveredColor.b  << m_hoveredColor.a;
  stream << m_pressedColor.r  << m_pressedColor.g
         << m_pressedColor.b  << m_pressedColor.a;
  stream << m_focusedColor.r  << m_focusedColor.g
         << m_focusedColor.b  << m_focusedColor.a;
  stream << m_disabledColor.r << m_disabledColor.g
         << m_disabledColor.b << m_disabledColor.a;
}

void
UIButton::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != 1) {
    return;
  }

  // Widget state
  setName(stream.readString());

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

  // Collider
  uint8 tag = 0;
  stream >> tag;
  if (tag != 0xFF) {
    switch (static_cast<ColliderType>(tag)) {
      case ColliderType::kCircle: {
        float cx, cy, radius;
        stream >> cx >> cy >> radius;
        setColliderCircle({cx, cy}, radius);
        break;
      }
      case ColliderType::kAABB: {
        float cx, cy, hx, hy;
        stream >> cx >> cy >> hx >> hy;
        setColliderAABB({cx, cy}, {hx, hy});
        break;
      }
      case ColliderType::kOBB: {
        float cx, cy, hx, hy;
        stream >> cx >> cy >> hx >> hy;
        setColliderOBB({cx, cy}, {hx, hy});
        break;
      }
      case ColliderType::kPoint: {
        float px, py;
        stream >> px >> py;
        setColliderPoint({px, py});
        break;
      }
      case ColliderType::kLine: {
        float sx, sy, ex, ey;
        stream >> sx >> sy >> ex >> ey;
        setColliderLine({sx, sy}, {ex, ey});
        break;
      }
    }
  } else {
    clearCollider();
  }

  // Button colour overrides
  stream >> m_normalColor.r   >> m_normalColor.g
         >> m_normalColor.b   >> m_normalColor.a;
  stream >> m_hoveredColor.r  >> m_hoveredColor.g
         >> m_hoveredColor.b  >> m_hoveredColor.a;
  stream >> m_pressedColor.r  >> m_pressedColor.g
         >> m_pressedColor.b  >> m_pressedColor.a;
  stream >> m_focusedColor.r  >> m_focusedColor.g
         >> m_focusedColor.b  >> m_focusedColor.a;
  stream >> m_disabledColor.r >> m_disabledColor.g
         >> m_disabledColor.b >> m_disabledColor.a;
}

} // namespace sfmx
