#include "core/physics/PhysicsSystem.h"

#include "scene/ColliderComponent.h"
#include "scene/RigidBodyComponent.h"
#include "scene/SceneNode.h"

namespace sfmx
{

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

PhysicsSystem::~PhysicsSystem() {
  m_colliders.clear();
  m_rigidBodies.clear();
  m_prevContacts.clear();
}

// -----------------------------------------------------------------------------
// Registration
// -----------------------------------------------------------------------------

void
PhysicsSystem::registerCollider(ColliderComponent* comp) {
  if (comp && std::find(m_colliders.begin(), m_colliders.end(), comp) == m_colliders.end())
    m_colliders.push_back(comp);
}

void
PhysicsSystem::unregisterCollider(ColliderComponent* comp) {
  m_colliders.erase(std::remove(m_colliders.begin(), m_colliders.end(), comp),
                    m_colliders.end());
}

void
PhysicsSystem::registerRigidBody(RigidBodyComponent* comp) {
  if (comp && std::find(m_rigidBodies.begin(), m_rigidBodies.end(), comp) == m_rigidBodies.end())
    m_rigidBodies.push_back(comp);
}

void
PhysicsSystem::unregisterRigidBody(RigidBodyComponent* comp) {
  m_rigidBodies.erase(std::remove(m_rigidBodies.begin(), m_rigidBodies.end(), comp),
                      m_rigidBodies.end());
}

// -----------------------------------------------------------------------------
// Pair key
// -----------------------------------------------------------------------------

/** @brief Normalised pair key so (a, b) == (b, a) */
PhysicsSystem::PairKey
PhysicsSystem::makeKey(ColliderComponent* a, ColliderComponent* b) {
  if (a < b) return {a, b};
  return {b, a};
}

// -----------------------------------------------------------------------------
// Layer filtering
// -----------------------------------------------------------------------------

/** @brief Returns true if two colliders should be tested for overlap */
bool
PhysicsSystem::shouldCollide(const ColliderComponent* a, const ColliderComponent* b) const {
  if (a == b) return false;
  const LayerMask layerA = static_cast<LayerMask>(a->getLayer());
  const LayerMask layerB = static_cast<LayerMask>(b->getLayer());
  return (layerA & b->getCollisionMask()) && (layerB & a->getCollisionMask());
}

// -----------------------------------------------------------------------------
// Narrow phase dispatch
// -----------------------------------------------------------------------------

/** @brief Runs the narrow-phase intersection test for a collider pair */
CollisionResult
PhysicsSystem::testNarrow(ColliderComponent* a, ColliderComponent* b) const {
  ICollider* ca = a->getCollider();
  ICollider* cb = b->getCollider();
  if (!ca || !cb) return {};

  const sf::Transform& wtA = a->getOwner()->transform().getWorldTransform();
  const sf::Transform& wtB = b->getOwner()->transform().getWorldTransform();

  return sfmx::intersect(*ca, wtA, *cb, wtB);
}

// -----------------------------------------------------------------------------
// Separation
// -----------------------------------------------------------------------------

/**
 * @brief Separates two overlapping colliders and resolves velocity.
 *
 * Static bodies (no RigidBodyComponent) are treated as immovable.
 * Triggers are skipped entirely (no separation).
 * Velocity along the collision normal is removed to prevent
 * objects from driving into each other on subsequent frames.
 */
void
PhysicsSystem::separate(ColliderComponent* a, ColliderComponent* b, const CollisionResult& cr) {
  if (a->isTrigger() || b->isTrigger()) return;

  auto* rbA = a->getOwner()->getComponent<RigidBodyComponent>();
  auto* rbB = b->getOwner()->getComponent<RigidBodyComponent>();

  const bool dynA = (rbA != nullptr);
  const bool dynB = (rbB != nullptr);

  if (!dynA && !dynB) return;

  const float invMassA = dynA ? rbA->getInverseMass() : 0.f;
  const float invMassB = dynB ? rbB->getInverseMass() : 0.f;
  const float total    = invMassA + invMassB;
  if (total < 1e-10f) return;

  const sf::Vector2f offset = cr.normal * cr.penetration;

  if (dynA)
    a->getOwner()->transform().move(-offset * (invMassA / total));
  if (dynB)
    b->getOwner()->transform().move( offset * (invMassB / total));

  // Resolve velocity along the collision normal so bodies don't keep
  // driving into each other on subsequent frames.
  if (dynA) {
    const sf::Vector2f velA = rbA->getVelocity();
    const float vDotN = velA.dot(cr.normal);
    if (vDotN > 0.f)
      rbA->setVelocity(velA - cr.normal * vDotN);
  }
  if (dynB) {
    const sf::Vector2f velB = rbB->getVelocity();
    const float vDotN = velB.dot(cr.normal);
    if (vDotN < 0.f)
      rbB->setVelocity(velB - cr.normal * vDotN);
  }
}

// -----------------------------------------------------------------------------
// Main step
// -----------------------------------------------------------------------------

void
PhysicsSystem::step(float dt) {
  // 1. Integrate rigid bodies
  for (auto* rb : m_rigidBodies) {
    rb->integrate(dt, m_gravity);
  }

  // 2. Detect collisions
  ContactSet currentContacts;

  for (size_t i = 0; i < m_colliders.size(); ++i) {
    for (size_t j = i + 1; j < m_colliders.size(); ++j) {
      auto* a = m_colliders[i];
      auto* b = m_colliders[j];
      if (!a->isEnabled() || !b->isEnabled()) continue;
      if (!shouldCollide(a, b)) continue;

      const CollisionResult cr = testNarrow(a, b);
      const PairKey key = makeKey(a, b);

      if (cr) {
        currentContacts.insert(key);

        const bool wasInContact = m_prevContacts.contains(key);
        if (wasInContact) {
          m_onStay(a, b);
        } else {
          m_onEnter(a, b);
        }

        separate(a, b, cr);
      }
    }
  }

  // 3. Detect exits
  for (const auto& key : m_prevContacts) {
    if (!currentContacts.contains(key)) {
      m_onExit(key.a, key.b);
    }
  }

  m_prevContacts = std::move(currentContacts);
}

} // namespace sfmx
