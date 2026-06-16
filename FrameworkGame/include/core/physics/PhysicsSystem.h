/************************************************************************/
/**
 * @file PhysicsSystem.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Physics system: integration, broad/narrow collision, separation.
 */
/************************************************************************/
#pragma once

#include "core/physics/Collider.h"
#include "core/platform/Prerequisites.h"
#include "utils/Module.h"

#include <SFML/System/Vector2.hpp>

namespace sfmx
{

class ColliderComponent;
class RigidBodyComponent;

// Layer / mask types

/** @brief Bit-field type used for physics layers and collision masks */
using LayerMask = uint64;

/** @brief Predefined physics layers (combinable as bit flags) */
enum class PhysicsLayer : LayerMask
{
  kNone    = 0,
  kDefault = 1 << 0,
  kUI      = 1 << 1,
  kPlayer  = 1 << 2,
  kEnemy   = 1 << 3,
  kWorld   = 1 << 4,
  kSensor  = 1 << 5,
};

// Physics constants

/**
 * @brief Default physics constants.
 *
 * kAllLayers  – every bit set (collide with everything).
 * kNoLayers   – no bits set (collide with nothing).
 * kDefaultGravity – Earth-like downward acceleration (pixels/s²).
 */
namespace Physics
{
  inline constexpr LayerMask   kAllLayers  = ~LayerMask(0);
  inline constexpr LayerMask   kNoLayers   = 0;
  inline constexpr sf::Vector2f kDefaultGravity = {0.f, 980.f};
} // namespace Physics

// Collision callback

/** @brief Signature for collision-event callbacks (enter / stay / exit) */
using CollisionEvent = Function<void(ColliderComponent*, ColliderComponent*)>;

// Physics System

/**
 * @brief Per-frame physics simulation step.
 *
 * Each call to step() performs:
 *   1. Rigid-body integration (gravity + velocity)
 *   2. Broad/narrow pair collision detection
 *   3. Separation (positional correction + velocity resolution)
 *   4. Callback firing (enter / stay / exit)
 *
 * Operates on registered ColliderComponent and RigidBodyComponent instances.
 */
class PhysicsSystem : public Module<PhysicsSystem>
{
 public:
  /** @brief Advance the simulation by dt seconds */
  void step(float dt);

  // Registration (called automatically by component ctor/dtor)

  /** @brief Register a collider for collision detection */
  void registerCollider(ColliderComponent* comp);
  /** @brief Unregister a collider (e.g. on destruction) */
  void unregisterCollider(ColliderComponent* comp);
  /** @brief Register a rigid body for integration */
  void registerRigidBody(RigidBodyComponent* comp);
  /** @brief Unregister a rigid body */
  void unregisterRigidBody(RigidBodyComponent* comp);

  // Callbacks

  /** @brief Called when two colliders first make contact */
  void setOnCollisionEnter(CollisionEvent cb) { m_onEnter = std::move(cb); }
  /** @brief Called each frame while two colliders stay in contact */
  void setOnCollisionStay(CollisionEvent cb)  { m_onStay  = std::move(cb); }
  /** @brief Called when two colliders cease to overlap */
  void setOnCollisionExit(CollisionEvent cb)  { m_onExit  = std::move(cb); }

  // Gravity

  /** @brief Set the global gravity vector (default: 980 px/s² downward) */
  void setGravity(sf::Vector2f g) { m_gravity = g; }
  /** @brief Current gravity vector */
  sf::Vector2f getGravity() const { return m_gravity; }

 protected:
  friend class Module<PhysicsSystem>;
  PhysicsSystem()  = default;
  ~PhysicsSystem() override;

 private:
  struct PairKey {
    ColliderComponent* a;
    ColliderComponent* b;

    bool operator==(const PairKey& o) const {
      return a == o.a && b == o.b;
    }
  };

  struct PairHash {
    size_t operator()(const PairKey& k) const {
      return reinterpret_cast<size_t>(k.a) ^
             (reinterpret_cast<size_t>(k.b) << 16);
    }
  };

  using ContactSet = UnorderedSet<PairKey, PairHash>;

  static PairKey makeKey(ColliderComponent* a, ColliderComponent* b);

  bool shouldCollide(const ColliderComponent* a, const ColliderComponent* b) const;
  CollisionResult testNarrow(ColliderComponent* a, ColliderComponent* b) const;
  void separate(ColliderComponent* a, ColliderComponent* b, const CollisionResult& cr);

  Vector<ColliderComponent*>   m_colliders;
  Vector<RigidBodyComponent*>  m_rigidBodies;
  ContactSet                   m_prevContacts;
  sf::Vector2f                 m_gravity = Physics::kDefaultGravity;

  CollisionEvent m_onEnter;
  CollisionEvent m_onStay;
  CollisionEvent m_onExit;
};

} // namespace sfmx
