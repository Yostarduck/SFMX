#pragma once

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/SceneTypes.h"
#include "utils/TypeTraits.h"

namespace sfmx
{
class SceneNode;

template<typename T>
NODISCARD ComponentTypeId
componentTypeId() {
  return TypeTraits<T>::getTypeId();
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
  explicit Component(SceneNode* owner)
    : m_owner(owner),
      m_nextComponent(nullptr),
      m_prevComponent(nullptr)
  {}

  virtual ~Component() = default;

  Component(const Component&) = delete;
  Component& operator=(const Component&) = delete;

  /** @brief The node this component is attached to (never null while live). */
  NODISCARD SceneNode* getOwner() const { return m_owner; }

  /** @brief This component's concrete-type id (see @ref componentTypeId). */
  NODISCARD virtual ComponentTypeId getTypeId() const = 0;

  /** @brief Next component on the same node's intrusive list, or nullptr. */
  NODISCARD Component* getNextComponent() const { return m_nextComponent; }
  /** @brief Previous component on the same node's intrusive list, or nullptr. */
  NODISCARD Component* getPrevComponent() const { return m_prevComponent; }

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

 private:
  friend class SceneNode;  // maintains the intrusive component list below

  Component* m_nextComponent;
  Component* m_prevComponent;
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
 public:
  NODISCARD static constexpr const ansichar*
  getTypeName() {
    return TypeTraits<Derived>::getTypeName();
  }

  NODISCARD ComponentTypeId
  getTypeId() const override {
    return componentTypeId<Derived>();
  }

 protected:
  explicit ComponentT(SceneNode* owner)
    : Component(owner)
  {}
};

}  // namespace sfmx
