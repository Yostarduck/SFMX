#include "scene/RigidBodyComponent.h"
#include "scene/SceneNode.h"
#include "scene/Transform.h"

namespace sfmx
{

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

} // namespace sfmx
