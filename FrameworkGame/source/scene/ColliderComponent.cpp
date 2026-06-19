#include "scene/ColliderComponent.h"
#include "scene/SceneNode.h"
#include "scene/Transform.h"

#include <SFML/Graphics/VertexArray.hpp>
#include <cmath>

namespace sfmx
{

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
  UUID uuid = m_collider->getTypeId();

  if (TypeTraits<CircleCollider>::getTypeId() == uuid) {
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
  }
  else if (TypeTraits<AABBCollider>::getTypeId() == uuid) {
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
  }
  else if (TypeTraits<OBBCollider>::getTypeId() == uuid) {
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
  }
  else if (TypeTraits<PointCollider>::getTypeId() == uuid) {
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
  else if (TypeTraits<LineCollider>::getTypeId() == uuid) {
    const auto* l  = static_cast<LineCollider*>(m_collider.get());
    va.resize(2);
    va[0].position = wt.transformPoint(l->getStart()); va[0].color = m_debugColor;
    va[1].position = wt.transformPoint(l->getEnd());   va[1].color = m_debugColor;
  }

  sf::RenderStates id = states;
  id.transform = sf::Transform::Identity;
  target.draw(va, id);
}

} // namespace sfmx
