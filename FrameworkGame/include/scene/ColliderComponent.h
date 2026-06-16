/************************************************************************/
/**
 * @file ColliderComponent.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Component that attaches a collider shape to a SceneNode.
 */
/************************************************************************/
#pragma once

#include "core/physics/Collider.h"
#include "core/physics/PhysicsSystem.h"
#include "scene/Component.h"
#include "utils/TypeTraits.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>

namespace sfmx
{

class SceneNode;

/**
 * @brief Attaches a collider shape to a SceneNode and participates in
 *        physics collision detection.
 *
 * The collider is created via one of the set*() methods (setCircle, setAABB,
 * etc.) which replace the internal shape.  The component auto-registers with
 * PhysicsSystem on construction and unregisters on destruction.
 */
class ColliderComponent : public ComponentT<ColliderComponent>
{
 public:
  explicit ColliderComponent(SceneNode* owner);
  ~ColliderComponent() override;

  // Shape setters (local space)

  /** @brief Replace the collider with a circle shape */
  void setCircle(sf::Vector2f localCenter, float radius);
  /** @brief Convenience: circle centered at origin */
  void setCircle(float radius)   { setCircle({0.f, 0.f}, radius); }
  /** @brief Replace the collider with an axis-aligned box */
  void setAABB(sf::Vector2f localCenter, sf::Vector2f halfSize);
  /** @brief Convenience: AABB centered at origin */
  void setAABB(sf::Vector2f halfSize)  { setAABB({0.f, 0.f}, halfSize); }
  /** @brief Replace the collider with an oriented box */
  void setOOBB(sf::Vector2f localCenter, sf::Vector2f halfSize);
  /** @brief Convenience: OOBB centered at origin */
  void setOOBB(sf::Vector2f halfSize)  { setOOBB({0.f, 0.f}, halfSize); }
  /** @brief Replace the collider with a point */
  void setPoint(sf::Vector2f localPos);
  /** @brief Replace the collider with a line segment */
  void setLine(sf::Vector2f localStart, sf::Vector2f localEnd);

  // Accessors

  /** @brief The current collider shape type */
  NODISCARD ColliderType  getColliderType() const;
  /** @brief Raw pointer to the internal collider (may be null) */
  NODISCARD Collider*     getCollider()       const { return m_collider.get(); }
  /** @brief Whether this collider acts as a trigger (no separation) */
  NODISCARD bool          isTrigger()         const { return m_trigger; }
  /** @brief The physics layer this collider belongs to */
  NODISCARD PhysicsLayer  getLayer()          const { return m_layer; }
  /** @brief Bitmask of layers this collider can collide with */
  NODISCARD LayerMask     getCollisionMask()  const { return m_collisionMask; }

  /** @brief Set whether this collider is a trigger */
  void setTrigger(bool v)       { m_trigger = v; }
  /** @brief Assign the physics layer */
  void setLayer(PhysicsLayer l) { m_layer = l; }
  /** @brief Set the collision mask (bitmask of PhysicsLayer values) */
  void setCollisionMask(LayerMask m) { m_collisionMask = m; }
  /** @brief Enable or disable this collider */
  void setEnabled(bool v)       { m_enabled = v; }
  /** @brief Whether this collider is enabled */
  NODISCARD bool isEnabled()    const { return m_enabled; }

  /** @brief Set the debug-render outline color */
  void setDebugColor(sf::Color c) { m_debugColor = c; }

  // Debug rendering

  /** @brief Draws a wireframe representation of the collider shape */
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
