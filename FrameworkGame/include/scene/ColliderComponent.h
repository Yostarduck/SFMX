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
  void setCircle(const sf::Vector2f& localCenter, float radius);
  /** @brief Convenience: circle centered at origin */
  void setCircle(float radius)   { setCircle({0.f, 0.f}, radius); }
  /** @brief Replace the collider with an axis-aligned box */
  void setAABB(const sf::Vector2f& localCenter, const sf::Vector2f& halfSize);
  /** @brief Convenience: AABB centered at origin */
  void setAABB(const sf::Vector2f& halfSize)  { setAABB({0.f, 0.f}, halfSize); }
  /** @brief Replace the collider with an oriented box */
  void setOBB(const sf::Vector2f& localCenter, const sf::Vector2f& halfSize);
  /** @brief Convenience: OBB centered at origin */
  void setOBB(const sf::Vector2f& halfSize)  { setOBB({0.f, 0.f}, halfSize); }
  /** @brief Replace the collider with a point */
  void setPoint(const sf::Vector2f& localPos);
  /** @brief Replace the collider with a line segment */
  void setLine(const sf::Vector2f& localStart, const sf::Vector2f& localEnd);

  // Accessors

  /** @brief The current collider shape type */
  // NODISCARD ColliderType  getColliderType() const;
  /** @brief Raw pointer to the internal collider (may be null) */
  NODISCARD ICollider*     getCollider()       const { return m_collider.get(); }
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
  void setCollisionMask(const LayerMask& m) { m_collisionMask = m; }
  /** @brief Enable or disable this collider */
  void setEnabled(bool v)       { m_enabled = v; }
  /** @brief Whether this collider is enabled */
  NODISCARD bool isEnabled()    const { return m_enabled; }

  /** @brief Set the debug-render outline color */
  void setDebugColor(const sf::Color& c) { m_debugColor = c; }
  /** @brief The debug-render outline color */
  NODISCARD const sf::Color& getDebugColor() const { return m_debugColor; }

  // Debug rendering

  /** @brief Draws a wireframe representation of the collider shape */
  void onDraw(sf::RenderTarget& target, sf::RenderStates states) const override;

  // Serialization

  /** @brief Serializes the shape (type tag + params), layer/mask, flags and debug color. */
  void onSerialize(DataStream& stream) const override;
  /** @brief Restores the state written by @ref onSerialize. */
  void onDeserialize(DataStream& stream) override;

 private:
  UniquePtr<ICollider> m_collider;

  PhysicsLayer m_layer         = PhysicsLayer::kDefault;
  LayerMask    m_collisionMask = Physics::kAllLayers;
  bool         m_trigger       = false;
  bool         m_enabled       = true;  //!< TODO: Blame @YoStarduck
  sf::Color    m_debugColor    = sf::Color::Green;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::ColliderComponent)
