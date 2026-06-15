#pragma once

#include "core/physics/Collider.h"
#include "core/platform/Prerequisites.h"
#include "utils/Module.h"

#include <SFML/System/Vector2.hpp>

namespace sfmx {

class ColliderComponent;
class RigidBodyComponent;

// ── Layer / mask types ────────────────────────────────────────────────────

using LayerMask = uint64;

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

// ── Physics constants ─────────────────────────────────────────────────────

namespace Physics
{
  inline constexpr LayerMask   kAllLayers  = ~LayerMask(0);
  inline constexpr LayerMask   kNoLayers   = 0;
  inline constexpr sf::Vector2f kDefaultGravity = {0.f, 980.f};
} // namespace Physics

// ── Physics collision event callbacks ─────────────────────────────────────

using CollisionEvent = Function<void(ColliderComponent*, ColliderComponent*)>;

// ── Physics System ────────────────────────────────────────────────────────

class PhysicsSystem : public Module<PhysicsSystem>
{
 public:
  void step(float dt);

  // ── Registration (called automatically by component ctor/dtor) ──────────
  void registerCollider(ColliderComponent* comp);
  void unregisterCollider(ColliderComponent* comp);
  void registerRigidBody(RigidBodyComponent* comp);
  void unregisterRigidBody(RigidBodyComponent* comp);

  // ── Callbacks ───────────────────────────────────────────────────────────
  void setOnCollisionEnter(CollisionEvent cb) { m_onEnter = std::move(cb); }
  void setOnCollisionStay(CollisionEvent cb)  { m_onStay  = std::move(cb); }
  void setOnCollisionExit(CollisionEvent cb)  { m_onExit  = std::move(cb); }

  // ── Gravity ─────────────────────────────────────────────────────────────
  void setGravity(sf::Vector2f g) { m_gravity = g; }
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
