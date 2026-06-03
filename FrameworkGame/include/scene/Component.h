#pragma once

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/SceneTypes.h"

namespace sfmx
{
class SceneNode;

namespace detail
{
/**
 * @brief Hand out the next never-before-used ComponentTypeId.
 *
 * The counter is an Atomic so ids can be assigned from any thread without a
 * lock; @ref relaxed ordering is sufficient because we only need atomicity and
 * uniqueness of the value, not ordering relative to other memory.
 */
NODISCARD inline ComponentTypeId
nextComponentTypeId() {
  static Atomic<ComponentTypeId> s_counter{0};
  return s_counter.fetch_add(1, std::memory_order_relaxed);
}
}  // namespace detail

/**
 * @brief The stable, unique id of a concrete component type @p T.
 *
 * Thread-safe: the per-@p T function-local static is initialised exactly once
 * (C++11 "magic statics"), and the underlying counter is atomic. No RTTI is
 * used, in keeping with the project coding standard.
 */
template<typename T>
NODISCARD ComponentTypeId
componentTypeId() {
  static const ComponentTypeId kId = detail::nextComponentTypeId();
  return kId;
}

/**
 * @brief Base class for data/behaviour attached to a SceneNode by composition.
 *
 * Concrete components live in per-type memory pools owned by the Scene; a node
 * holds non-owning Component* pointers into those pools. A component never
 * outlives its owning node. Subclasses add behaviour by overriding @ref
 * onUpdate / @ref onDraw; the owning node's traversal calls them.
 *
 * Prefer deriving from @ref ComponentT, which fills in the type id for you.
 */
class Component
{
 public:
  Component(SceneNode* owner, ComponentTypeId typeId)
    : m_owner(owner),
      m_typeId(typeId)
  {}

  virtual ~Component() = default;

  Component(const Component&) = delete;
  Component& operator=(const Component&) = delete;

  /** @brief The node this component is attached to (never null while live). */
  NODISCARD SceneNode* getOwner() const { return m_owner; }

  /** @brief This component's concrete-type id (see @ref componentTypeId). */
  NODISCARD ComponentTypeId getTypeId() const { return m_typeId; }

  /**
   * @brief Per-frame update hook, called by the owning node's traversal.
   * @param deltaTime Seconds elapsed since the previous frame.
   */
  virtual void
  onUpdate(float deltaTime) { SFMX_PARAMETER_UNUSED(deltaTime); }

  /**
   * @brief Draw hook, called by the owning node's traversal.
   *
   * @param target States the surface to draw onto.
   * @param states Render states carrying the accumulated world transform of
   *               the owning node; pass them straight to @c target.draw.
   */
  virtual void
  onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
    SFMX_PARAMETER_UNUSED(target);
    SFMX_PARAMETER_UNUSED(states);
  }

 protected:
  SceneNode* m_owner;
  ComponentTypeId m_typeId;
};

/**
 * @brief CRTP helper that supplies a concrete component's type id.
 *
 * Derive as @c class Velocity : public ComponentT<Velocity> and provide a
 * constructor taking @c (SceneNode* owner, ...); the owner is forwarded to the
 * base together with @c componentTypeId<Velocity>().
 */
template<typename Derived>
class ComponentT : public Component
{
 protected:
  explicit ComponentT(SceneNode* owner)
    : Component(owner, componentTypeId<Derived>())
  {}
};

}  // namespace sfmx
