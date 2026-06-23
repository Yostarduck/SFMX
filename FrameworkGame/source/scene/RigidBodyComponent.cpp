#include "scene/RigidBodyComponent.h"
#include "scene/SceneNode.h"
#include "scene/Transform.h"

#include "core/DataStream.h"

namespace sfmx
{

namespace {
/** @brief RigidBodyComponent blob layout version; bump on format changes. */
constexpr uint32 kRigidBodyComponentVersion = 1;
} // namespace

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

RigidBodyComponent::RigidBodyComponent(SceneNode* owner)
  : ComponentT<RigidBodyComponent>(owner)
{
  if (PhysicsSystem::isStarted())
    PhysicsSystem::instance().registerRigidBody(this);
}

RigidBodyComponent::~RigidBodyComponent() {
  if (PhysicsSystem::isStarted())
    PhysicsSystem::instance().unregisterRigidBody(this);
}

// -----------------------------------------------------------------------------
// Integration
// -----------------------------------------------------------------------------

void
RigidBodyComponent::integrate(float dt, sf::Vector2f gravity) {
  // Apply accumulated forces
  m_velocity += m_forceAccum * getInverseMass() * dt;

  // Apply gravity
  m_velocity += gravity * m_gravityScale * dt;

  // Clear forces for next frame
  m_forceAccum = {0.f, 0.f};

  // Integrate position
  m_owner->transform().move(m_velocity * dt);
}

// -----------------------------------------------------------------------------
// Serialization
// -----------------------------------------------------------------------------

void
RigidBodyComponent::onSerialize(DataStream& stream) const {
  stream << kRigidBodyComponentVersion;
  stream << m_velocity.x << m_velocity.y;
  stream << m_mass;
  stream << m_gravityScale;
  // m_forceAccum is transient per-frame scratch — intentionally not serialized.
}

void
RigidBodyComponent::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != kRigidBodyComponentVersion) {
    return;  // unknown version: leave defaults rather than misread bytes
  }

  float vx = 0.f;
  float vy = 0.f;
  float mass = 1.f;
  float gravityScale = 1.f;
  stream >> vx >> vy >> mass >> gravityScale;
  m_velocity     = {vx, vy};
  m_mass         = mass;
  m_gravityScale = gravityScale;
}

} // namespace sfmx
