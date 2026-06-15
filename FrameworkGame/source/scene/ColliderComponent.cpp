#include "scene/ColliderComponent.h"
#include "scene/SceneNode.h"
#include "scene/Transform.h"

#include <SFML/Graphics/VertexArray.hpp>
#include <cmath>

namespace sfmx {

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

// ── Shape setters ────────────────────────────────────────────────────────

void
ColliderComponent::setCircle(sf::Vector2f localCenter, float radius) {
  m_collider = UniquePtr<Collider>(new CircleCollider(localCenter, radius));
}

void
ColliderComponent::setAABB(sf::Vector2f localCenter, sf::Vector2f halfSize) {
  m_collider = UniquePtr<Collider>(new AABBCollider(localCenter, halfSize));
}

void
ColliderComponent::setOOBB(sf::Vector2f localCenter, sf::Vector2f halfSize) {
  m_collider = UniquePtr<Collider>(new OOBBCollider(localCenter, halfSize));
}

void
ColliderComponent::setPoint(sf::Vector2f localPos) {
  m_collider = UniquePtr<Collider>(new PointCollider(localPos));
}

void
ColliderComponent::setLine(sf::Vector2f localStart, sf::Vector2f localEnd) {
  m_collider = UniquePtr<Collider>(new LineCollider(localStart, localEnd));
}

// ── Accessors ────────────────────────────────────────────────────────────

ColliderType
ColliderComponent::getColliderType() const {
  return m_collider ? m_collider->getType() : ColliderType::kCircle;
}

// ── Debug rendering ──────────────────────────────────────────────────────

void
ColliderComponent::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!m_collider) return;

  const sf::Transform& wt = m_owner->transform().getWorldTransform();
  sf::VertexArray va(sf::PrimitiveType::LineStrip, 0);

  switch (m_collider->getType()) {
    case ColliderType::kCircle: {
      const auto* c  = static_cast<CircleCollider*>(m_collider.get());
      const sf::Vector2f origin = wt.transformPoint({0.f, 0.f});
      const float sx = length(wt.transformPoint({1.f, 0.f}) - origin);
      const float sy = length(wt.transformPoint({0.f, 1.f}) - origin);
      const sf::Vector2f center = wt.transformPoint(c->getCenter());
      const float radius = c->getRadius() * (sx + sy) * 0.5f;
      constexpr int segs = 32;
      va.resize(segs + 1);
      for (int i = 0; i <= segs; ++i) {
        const float a = 6.2831855f * i / segs;
        va[i].position = center + sf::Vector2f{std::cos(a) * radius, std::sin(a) * radius};
        va[i].color    = m_debugColor;
      }
      sf::RenderStates id = states;
      id.transform = sf::Transform::Identity;
      target.draw(va, id);
      return;
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
    case ColliderType::kOOBB: {
      const auto* o  = static_cast<OOBBCollider*>(m_collider.get());
      const sf::Vector2f ori = wt.transformPoint({0.f, 0.f});
      const sf::Vector2f ax  = normalize(wt.transformPoint({1.f, 0.f}) - ori);
      const sf::Vector2f ay  = normalize(wt.transformPoint({0.f, 1.f}) - ori);
      const float sx = length(wt.transformPoint({1.f, 0.f}) - ori);
      const float sy = length(wt.transformPoint({0.f, 1.f}) - ori);
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
      sf::RenderStates id = sf::Transform::Identity;
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

} // namespace sfmx
