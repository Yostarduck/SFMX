#pragma once

#include "core/physics/Collider.h"
#include "core/physics/PhysicsSystem.h"
#include "scene/Component.h"
#include "utils/TypeTraits.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>

namespace sfmx {

class SceneNode;

class ColliderComponent : public ComponentT<ColliderComponent>
{
 public:
  explicit ColliderComponent(SceneNode* owner);
  ~ColliderComponent() override;

  // ── Shape setters (local space) ───────────────────────────────────────
  void setCircle(sf::Vector2f localCenter, float radius);
  void setCircle(float radius)   { setCircle({0.f, 0.f}, radius); }
  void setAABB(sf::Vector2f localCenter, sf::Vector2f halfSize);
  void setAABB(sf::Vector2f halfSize)  { setAABB({0.f, 0.f}, halfSize); }
  void setOOBB(sf::Vector2f localCenter, sf::Vector2f halfSize);
  void setOOBB(sf::Vector2f halfSize)  { setOOBB({0.f, 0.f}, halfSize); }
  void setPoint(sf::Vector2f localPos);
  void setLine(sf::Vector2f localStart, sf::Vector2f localEnd);

  // ── Accessors ─────────────────────────────────────────────────────────
  NODISCARD ColliderType  getColliderType() const;
  NODISCARD Collider*     getCollider()       const { return m_collider.get(); }
  NODISCARD bool          isTrigger()         const { return m_trigger; }
  NODISCARD PhysicsLayer  getLayer()          const { return m_layer; }
  NODISCARD LayerMask     getCollisionMask()  const { return m_collisionMask; }

  void setTrigger(bool v)       { m_trigger = v; }
  void setLayer(PhysicsLayer l) { m_layer = l; }
  void setCollisionMask(LayerMask m) { m_collisionMask = m; }
  void setEnabled(bool v)       { m_enabled = v; }
  NODISCARD bool isEnabled()    const { return m_enabled; }

  void setDebugColor(sf::Color c) { m_debugColor = c; }

  // ── Debug rendering ───────────────────────────────────────────────────
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

 private:
  UniquePtr<Collider> m_collider;

  PhysicsLayer m_layer         = PhysicsLayer::kDefault;
  LayerMask    m_collisionMask = Physics::kAllLayers;
  bool         m_trigger       = false;
  bool         m_enabled       = true;
  sf::Color    m_debugColor    = sf::Color::Green;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::ColliderComponent)
