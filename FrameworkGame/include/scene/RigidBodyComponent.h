#pragma once

#include "core/physics/PhysicsSystem.h"
#include "scene/Component.h"
#include "utils/TypeTraits.h"

#include <SFML/System/Vector2.hpp>

namespace sfmx {

class RigidBodyComponent : public ComponentT<RigidBodyComponent>
{
 public:
  explicit RigidBodyComponent(SceneNode* owner);
  ~RigidBodyComponent() override;

  // ── Properties ─────────────────────────────────────────────────────────
  void setVelocity(sf::Vector2f v)    { m_velocity = v; }
  void setMass(float m)               { m_mass = std::max(m, 0.f); }
  void setGravityScale(float s)       { m_gravityScale = s; }

  NODISCARD sf::Vector2f getVelocity()     const { return m_velocity; }
  NODISCARD float         getMass()         const { return m_mass; }
  NODISCARD float         getInverseMass()  const { return (m_mass > 0.f) ? 1.f / m_mass : 0.f; }
  NODISCARD float         getGravityScale() const { return m_gravityScale; }

  void addForce(sf::Vector2f f)       { m_forceAccum += f; }
  void addImpulse(sf::Vector2f j)     { m_velocity += j * getInverseMass(); }

  // Called by PhysicsSystem
  void integrate(float dt, sf::Vector2f gravity);

 private:
  sf::Vector2f m_velocity      = {0.f, 0.f};
  sf::Vector2f m_forceAccum    = {0.f, 0.f};
  float        m_mass          = 1.f;
  float        m_gravityScale  = 1.f;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::RigidBodyComponent)
