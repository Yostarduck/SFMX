#include "scene/ColliderComponent.h"
#include "scene/SceneNode.h"
#include "scene/Transform.h"

#include <SFML/Graphics/VertexArray.hpp>
#include <cmath>

#include "core/DataStream.h"

namespace sfmx
{

namespace {
/** @brief ColliderComponent blob layout version; bump on format changes. */
constexpr uint32 kColliderComponentVersion = 1;
} // namespace

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

ColliderComponent::ColliderComponent(SceneNode* owner)
  : ComponentT<ColliderComponent>(owner)
  , m_collider(new CircleCollider())
{
  if (PhysicsSystem::isStarted())
    PhysicsSystem::instance().registerCollider(this);
}

ColliderComponent::~ColliderComponent() {
  if (PhysicsSystem::isStarted())
    PhysicsSystem::instance().unregisterCollider(this);
}

// -----------------------------------------------------------------------------
// Shape setters
// -----------------------------------------------------------------------------

void
ColliderComponent::setCircle(const sf::Vector2f& localCenter, float radius) {
  m_collider = UniquePtr<ICollider>(new CircleCollider(localCenter, radius));
}

void
ColliderComponent::setAABB(const sf::Vector2f& localCenter, const sf::Vector2f& halfSize) {
  m_collider = UniquePtr<ICollider>(new AABBCollider(localCenter, halfSize));
}

void
ColliderComponent::setOBB(const sf::Vector2f& localCenter, const sf::Vector2f& halfSize) {
  m_collider = UniquePtr<ICollider>(new OBBCollider(localCenter, halfSize));
}

void
ColliderComponent::setPoint(const sf::Vector2f& localPos) {
  m_collider = UniquePtr<ICollider>(new PointCollider(localPos));
}

void
ColliderComponent::setLine(const sf::Vector2f& localStart, const sf::Vector2f& localEnd) {
  m_collider = UniquePtr<ICollider>(new LineCollider(localStart, localEnd));
}

// -----------------------------------------------------------------------------
// Accessors
// -----------------------------------------------------------------------------

/** @brief Returns the type of the currently held collider shape */
// ColliderType
// ColliderComponent::getColliderType() const {
//   return m_collider ? m_collider->getType() : ColliderType::kCircle;
// }

// -----------------------------------------------------------------------------
// Debug rendering
// -----------------------------------------------------------------------------

void
ColliderComponent::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!m_collider) return;

  const sf::Transform& wt = m_owner->transform().getWorldTransform();
  sf::VertexArray va(sf::PrimitiveType::LineStrip, 0);

  switch (m_collider->getType()) {
    case ColliderType::kCircle: {
      const auto* c  = static_cast<CircleCollider*>(m_collider.get());
      const sf::Vector2f origin = wt.transformPoint({0.f, 0.f});
      const float sx = (wt.transformPoint({1.f, 0.f}) - origin).length();
      const float sy = (wt.transformPoint({0.f, 1.f}) - origin).length();
      const sf::Vector2f center = wt.transformPoint(c->getCenter());
      const float radius = c->getRadius() * (sx + sy) * 0.5f;
      constexpr int segs = 32;
      va.resize(segs + 1);
      for (int i = 0; i <= segs; ++i) {
        const float a = 6.2831855f * i / segs;
        va[i].position = center + sf::Vector2f{std::cos(a) * radius, std::sin(a) * radius};
        va[i].color    = m_debugColor;
      }
      break;
    }
    case ColliderType::kAABB: {
      const auto* a  = static_cast<AABBCollider*>(m_collider.get());
      const auto hs  = a->getHalfSize();
      const sf::Vector2f c[4] = {
        wt.transformPoint(a->getCenter() + sf::Vector2f{-hs.x, -hs.y}),
        wt.transformPoint(a->getCenter() + sf::Vector2f{ hs.x, -hs.y}),
        wt.transformPoint(a->getCenter() + sf::Vector2f{ hs.x,  hs.y}),
        wt.transformPoint(a->getCenter() + sf::Vector2f{-hs.x,  hs.y})
      };
      va.resize(5);
      for (int i = 0; i < 4; ++i) { va[i].position = c[i]; va[i].color = m_debugColor; }
      va[4] = va[0];
      break;
    }
    case ColliderType::kOBB: {
      const auto* o  = static_cast<OBBCollider*>(m_collider.get());
      const sf::Vector2f ori = wt.transformPoint({0.f, 0.f});
      const sf::Vector2f ax  = (wt.transformPoint({1.f, 0.f}) - ori).normalized();
      const sf::Vector2f ay  = (wt.transformPoint({0.f, 1.f}) - ori).normalized();
      const float sx = (wt.transformPoint({1.f, 0.f}) - ori).length();
      const float sy = (wt.transformPoint({0.f, 1.f}) - ori).length();
      const sf::Vector2f center = wt.transformPoint(o->getCenter());
      const auto hs = o->getHalfSize();
      const sf::Vector2f ex = ax * hs.x * sx, ey = ay * hs.y * sy;
      const sf::Vector2f cr[4] = {
        center - ex - ey, center + ex - ey, center + ex + ey, center - ex + ey
      };
      va.resize(5);
      for (int i = 0; i < 4; ++i) { va[i].position = cr[i]; va[i].color = m_debugColor; }
      va[4] = va[0];
      break;
    }
    case ColliderType::kPoint: {
      const auto* p  = static_cast<PointCollider*>(m_collider.get());
      const sf::Vector2f pt = wt.transformPoint(p->getPoint());
      va.resize(2);
      va[0].position = pt + sf::Vector2f{-3.f, -3.f}; va[0].color = m_debugColor;
      va[1].position = pt + sf::Vector2f{ 3.f,  3.f}; va[1].color = m_debugColor;
      sf::VertexArray va2(sf::PrimitiveType::LineStrip, 2);
      va2[0].position = pt + sf::Vector2f{-3.f,  3.f}; va2[0].color = m_debugColor;
      va2[1].position = pt + sf::Vector2f{ 3.f, -3.f}; va2[1].color = m_debugColor;
      sf::RenderStates id = states;
      id.transform = sf::Transform::Identity;
      target.draw(va, id);
      target.draw(va2, id);
      return;
    }
    case ColliderType::kLine: {
      const auto* l  = static_cast<LineCollider*>(m_collider.get());
      va.resize(2);
      va[0].position = wt.transformPoint(l->getStart()); va[0].color = m_debugColor;
      va[1].position = wt.transformPoint(l->getEnd());   va[1].color = m_debugColor;
      break;
    }
  }

  sf::RenderStates id = states;
  id.transform = sf::Transform::Identity;
  target.draw(va, id);
}

// -----------------------------------------------------------------------------
// Serialization
// -----------------------------------------------------------------------------

void
ColliderComponent::onSerialize(DataStream& stream) const {
  stream << kColliderComponentVersion;

  const ColliderType type = (nullptr != m_collider) ? m_collider->getType()
                                                    : ColliderType::kCircle;
  stream << static_cast<uint8>(type);

  // Shape params in local space (at most two Vector2f: center + extents/end).
  switch (type) {
    case ColliderType::kCircle: {
      const auto* c = static_cast<const CircleCollider*>(m_collider.get());
      const sf::Vector2f center = c->getCenter();
      stream << center.x << center.y << c->getRadius();
      break;
    }
    case ColliderType::kAABB: {
      const auto* a = static_cast<const AABBCollider*>(m_collider.get());
      const sf::Vector2f center = a->getCenter();
      const sf::Vector2f half   = a->getHalfSize();
      stream << center.x << center.y << half.x << half.y;
      break;
    }
    case ColliderType::kOBB: {
      const auto* o = static_cast<const OBBCollider*>(m_collider.get());
      const sf::Vector2f center = o->getCenter();
      const sf::Vector2f half   = o->getHalfSize();
      stream << center.x << center.y << half.x << half.y;
      break;
    }
    case ColliderType::kPoint: {
      const auto* p = static_cast<const PointCollider*>(m_collider.get());
      const sf::Vector2f pt = p->getPoint();
      stream << pt.x << pt.y;
      break;
    }
    case ColliderType::kLine: {
      const auto* l = static_cast<const LineCollider*>(m_collider.get());
      const sf::Vector2f start = l->getStart();
      const sf::Vector2f end   = l->getEnd();
      stream << start.x << start.y << end.x << end.y;
      break;
    }
  }

  stream << static_cast<LayerMask>(m_layer);
  stream << m_collisionMask;
  stream << static_cast<uint8>(m_trigger ? 1 : 0);
  stream << static_cast<uint8>(m_enabled ? 1 : 0);
  stream << m_debugColor.r << m_debugColor.g << m_debugColor.b << m_debugColor.a;
}

void
ColliderComponent::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != kColliderComponentVersion) {
    return;  // unknown version: leave defaults rather than misread bytes
  }

  uint8 typeTag = 0;
  stream >> typeTag;
  switch (static_cast<ColliderType>(typeTag)) {
    case ColliderType::kCircle: {
      float cx = 0.f;
      float cy = 0.f;
      float radius = 0.f;
      stream >> cx >> cy >> radius;
      setCircle({cx, cy}, radius);
      break;
    }
    case ColliderType::kAABB: {
      float cx = 0.f;
      float cy = 0.f;
      float hx = 0.f;
      float hy = 0.f;
      stream >> cx >> cy >> hx >> hy;
      setAABB({cx, cy}, {hx, hy});
      break;
    }
    case ColliderType::kOBB: {
      float cx = 0.f;
      float cy = 0.f;
      float hx = 0.f;
      float hy = 0.f;
      stream >> cx >> cy >> hx >> hy;
      setOBB({cx, cy}, {hx, hy});
      break;
    }
    case ColliderType::kPoint: {
      float px = 0.f;
      float py = 0.f;
      stream >> px >> py;
      setPoint({px, py});
      break;
    }
    case ColliderType::kLine: {
      float sx = 0.f;
      float sy = 0.f;
      float ex = 0.f;
      float ey = 0.f;
      stream >> sx >> sy >> ex >> ey;
      setLine({sx, sy}, {ex, ey});
      break;
    }
  }

  LayerMask layer = 0;
  LayerMask mask = 0;
  stream >> layer;
  stream >> mask;
  m_layer         = static_cast<PhysicsLayer>(layer);
  m_collisionMask = mask;

  uint8 trigger = 0;
  uint8 enabled = 1;
  stream >> trigger >> enabled;
  m_trigger = trigger != 0;
  m_enabled = enabled != 0;

  uint8 r = 0;
  uint8 g = 0;
  uint8 b = 0;
  uint8 a = 255;
  stream >> r >> g >> b >> a;
  m_debugColor = sf::Color(r, g, b, a);
}

} // namespace sfmx
