#pragma once

#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "utils/TypeTraits.h"

namespace sfmx
{
class SceneNode;

/**
 * @brief The transform of a node, relative to its parent (local space).
 *
 * Conceptually a component, but stored inline and always-present on every
 * SceneNode (it is hot and universal), so it is not pool-allocated and not in
 * the node's component list. It owns the local SRT (via sf::Transformable) and
 * caches the local-to-world matrix, recomputing it only when dirty.
 *
 * All mutation goes through the wrapper setters below so the dirty flag is
 * always maintained; the raw sf::Transformable is intentionally not exposed.
 */
class Transform : public ComponentT<Transform>
{
 public:
  explicit Transform(SceneNode* owner);

  /** @brief Set the local position; marks this subtree's world cache dirty. */
  void setPosition(const sf::Vector2f& position);
  /** @brief Translate by @p offset in local space; marks the subtree dirty. */
  void move(const sf::Vector2f& offset);
  /** @brief Set the local rotation; marks this subtree's world cache dirty. */
  void setRotation(sf::Angle angle);
  /** @brief Rotate by @p angle; marks this subtree's world cache dirty. */
  void rotate(sf::Angle angle);
  /** @brief Set the local scale; marks this subtree's world cache dirty. */
  void setScale(const sf::Vector2f& factors);
  /** @brief Scale by @p factors; marks this subtree's world cache dirty. */
  void scale(const sf::Vector2f& factors);
  
  NODISCARD sf::Vector2f getPosition() const { return m_local.getPosition(); }
  NODISCARD sf::Angle getRotation() const { return m_local.getRotation(); }
  NODISCARD sf::Vector2f getScale() const { return m_local.getScale(); }

  /** @brief Local-to-parent matrix. */
  NODISCARD const sf::Transform&
  getLocalTransform() const { return m_local.getTransform(); }

  /**
   * @brief Local-to-world matrix, @c world = parent.world * local.
   *
   * Recomputes and caches the matrix only when the dirty flag is set, walking
   * one step up to the parent (whose own world transform is likewise cached);
   * otherwise returns the cached value. Non-const because it may refresh the
   * cache.
   */
  NODISCARD const sf::Transform& getWorldTransform();

  /**
   * @brief Invalidate this node's cached world transform and that of every
   *        descendant (their world transforms depend on this one).
   */
  void markDirty();

 private:
  sf::Transformable m_local;
  sf::Transform m_worldCache;
  bool m_worldDirty;
};

}  // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::Transform)
