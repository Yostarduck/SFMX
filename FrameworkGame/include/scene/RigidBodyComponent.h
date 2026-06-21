/************************************************************************/
/**
 * @file RigidBodyComponent.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Adds velocity, mass, and force-based motion to a SceneNode.
 */
/************************************************************************/
#pragma once

#include "core/physics/PhysicsSystem.h"
#include "scene/Component.h"
#include "utils/TypeTraits.h"

#include <SFML/System/Vector2.hpp>

namespace sfmx
{

/**
 * @brief Gives a SceneNode physics-driven movement.
 *
 * RigidBodyComponent integrates velocity and forces each frame.
 * It auto-registers with PhysicsSystem on construction and
 * unregisters on destruction.
 */
class RigidBodyComponent : public ComponentT<RigidBodyComponent>
{
 public:
  explicit RigidBodyComponent(SceneNode* owner);
  ~RigidBodyComponent() override;

  // Properties

  /** @brief Set the current velocity (pixels / second) */
  void setVelocity(sf::Vector2f v)    { m_velocity = v; }
  /** @brief Set the mass (must be >= 0; 0 = infinite mass) */
  void setMass(float m)               { m_mass = std::max(m, 0.f); }
  /** @brief Multiplier applied to gravity (1.0 = normal) */
  void setGravityScale(float s)       { m_gravityScale = s; }

  /** @brief Current velocity */
  NODISCARD sf::Vector2f getVelocity()     const { return m_velocity; }
  /** @brief Mass value */
  NODISCARD float         getMass()         const { return m_mass; }
  /** @brief 1/mass (0 for infinite mass) */
  NODISCARD float         getInverseMass()  const { return (m_mass > 0.f) ? 1.f / m_mass : 0.f; }
  /** @brief Gravity scale factor */
  NODISCARD float         getGravityScale() const { return m_gravityScale; }

  /** @brief Accumulate a force to apply during the next integration step */
  void addForce(sf::Vector2f f)       { m_forceAccum += f; }
  /** @brief Instant velocity change (force × 1/mass) */
  void addImpulse(sf::Vector2f j)     { m_velocity += j * getInverseMass(); }

  // Internal — called by PhysicsSystem

  /** @brief Apply accumulated forces, gravity, and advance position */
  void integrate(float dt, sf::Vector2f gravity);

 private:
  sf::Vector2f m_velocity      = {0.f, 0.f};
  sf::Vector2f m_forceAccum    = {0.f, 0.f};
  float        m_mass          = 1.f;
  float        m_gravityScale  = 1.f;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::RigidBodyComponent)
