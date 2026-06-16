#pragma once

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "utils/MemoryPoolHandler.h"
#include "scene/SceneTypes.h"
#include "scene/Transform.h"
#include "utils/TypeTraits.h"

namespace sfmx
{
class Scene;

/**
 * @brief One node in the scene hierarchy.
 *
 * A node carries a stable @ref NodeId, a fixed-length name, enable/visible
 * flags, an inline @ref Transform, an intrusive list of attached
 * @ref Component "components", and links to its parent and children. Nodes are
 * owned by a Scene's memory pool and never relocate, so the hierarchy and
 * component lists are threaded through plain non-owning pointers stored inside
 * the pooled objects themselves - no per-node or per-component heap allocation.
 *
 * Children form a doubly-linked sibling list (@c m_firstChild / @c m_lastChild
 * on the parent, @c m_prevSibling / @c m_nextSibling on each child); components
 * are threaded the same way. Both give O(1) attach/detach and zero auxiliary
 * allocation, at the cost of no O(1) indexed access (which a scene graph does
 * not need).
 *
 * Behaviour and drawing are added by attaching components, not by subclassing:
 * the node pool stores SceneNode by value, so a derived type could not be
 * pooled as the base. The per-node traversal drives each component's
 * @ref Component::onUpdate / @ref Component::onDraw.
 *
 * Nodes are created and destroyed only through their owning @ref Scene.
 */
class SceneNode
{
 public:
  SceneNode(NodeId id, StringView name, Scene* scene);
  virtual ~SceneNode();

  SceneNode(const SceneNode&) = delete;
  SceneNode& operator=(const SceneNode&) = delete;

  // -- Identity & state ------------------------------------------------------
  NODISCARD NodeId getId() const { return m_id; }
  NODISCARD const char* getName() const { return m_name.data(); }
  void setName(StringView name);
  NODISCARD bool isEnabled() const { return m_enabled; }
  void setEnabled(bool enabled) { m_enabled = enabled; }
  NODISCARD bool isVisible() const { return m_visible; }
  void setVisible(bool visible) { m_visible = visible; }

  /** @brief True only if this node and every ancestor are enabled. */
  NODISCARD bool isEnabledInHierarchy() const;

  // -- Transform -------------------------------------------------------------
  NODISCARD Transform& transform() { return m_transform; }
  NODISCARD const Transform& transform() const { return m_transform; }

  /** @brief Convenience forwarder to the inline transform's world matrix. */
  NODISCARD const sf::Transform&
  getWorldTransform() { return m_transform.getWorldTransform(); }

  // -- Hierarchy queries -----------------------------------------------------
  NODISCARD SceneNode* getParent() const { return m_parent; }
  NODISCARD SceneNode* getFirstChild() const { return m_firstChild; }
  NODISCARD SceneNode* getLastChild() const { return m_lastChild; }
  NODISCARD SceneNode* getNextSibling() const { return m_nextSibling; }
  NODISCARD SceneNode* getPrevSibling() const { return m_prevSibling; }

  /** @brief Number of direct children (walks the sibling list, O(n)). */
  NODISCARD size_t getChildCount() const;

  /** @brief True if this node is an ancestor of @p node (cycle guard). */
  NODISCARD bool isAncestorOf(const SceneNode* node) const;

  /** @brief First direct child whose name equals @p name, or nullptr. */
  NODISCARD SceneNode* findChild(StringView name) const;

  // -- Hierarchy mutation ----------------------------------------------------
  /** @brief Create a child node through the owning Scene; returns the child. */
  SceneNode* createChild(StringView name);

  /**
   * @brief Move this node under @p newParent.
   *
   * Rejects cycles (a node may not become a child of its own descendant). When
   * @p keepWorldTransform is true, the node's world-space origin is preserved
   * so it does not visually jump. NOTE: only the origin position is preserved;
   * rotation/scale become relative to the new parent, since sf::Transformable
   * cannot represent an arbitrary decomposed matrix.
   */
  void reparent(SceneNode* newParent, bool keepWorldTransform = true);

  /** @brief Remove this node from its parent's child list (does not destroy). */
  void detachFromParent();

  // -- Components ------------------------------------------------------------
  /**
   * @brief Construct a component of type @p T in its pool and attach it.
   *
   * @p T must derive from @ref Component and take @c (SceneNode* owner, Args...)
   * in its constructor. Returns nullptr if that component pool is full.
   * Defined in Scene.h, where the component pools are complete.
   */
  template<typename T, typename... Args>
  T* addComponent(Args&&... args);

  /** @brief First attached component of type @p T, or nullptr. */
  template<typename T>
  NODISCARD T* getComponent() const;

  /** @brief Whether a component of type @p T is attached. */
  template<typename T>
  NODISCARD bool hasComponent() const { return nullptr != getComponent<T>(); }

  /** @brief Detach and return the first component of type @p T to its pool. */
  template<typename T>
  void removeComponent();

  /** @brief First component in the intrusive list (or nullptr). */
  NODISCARD Component* getFirstComponent() const { return m_firstComponent; }

  // -- Traversal -------------------------------------------------------------
  /**
   * @brief Advance this node and its enabled subtree by @p deltaTime.
   *
   * Disabled nodes (and their whole subtree) are skipped.
   */
  void update(float deltaTime);

  /**
   * @brief Draw this node's visible subtree.
   *
   * Composes the local transform into @p states, draws the node's components,
   * then recurses into children. Invisible nodes (and their subtree) are
   * skipped.
   */
  void draw(sf::RenderTarget& target, sf::RenderStates states) const;

 private:
  friend class Scene;  // node creation, linking, registry, teardown

  /** @brief Link @p child at the end of this node's child list. */
  void appendChild(SceneNode* child);
  /** @brief Link @p component at the end of this node's component list. */
  void linkComponent(Component* component);
  /** @brief Splice @p component out of this node's component list. */
  void unlinkComponent(Component* component);

  NodeId m_id;
  Array<char, kMaxNameLength> m_name;
  bool m_enabled;
  bool m_visible;
  SceneNode* m_parent;
  SceneNode* m_firstChild;
  SceneNode* m_lastChild;
  SceneNode* m_prevSibling;
  SceneNode* m_nextSibling;
  Scene* m_scene;
  Transform m_transform;
  Component* m_firstComponent;
  Component* m_lastComponent;
};

template<typename T, typename... Args>
T*
SceneNode::addComponent(Args&&... args) {
  static_assert(std::is_base_of<Component, T>::value,
                "addComponent<T>: T must derive from Component");
  MemoryPool<T>& pool = MemoryPoolHandler::instance().pool<T>();
  T* component = pool.allocate(this, std::forward<Args>(args)...);
  if (nullptr != component) {
    linkComponent(component);
  }
  return component;
}

template<typename T>
NODISCARD T*
SceneNode::getComponent() const {
  const ComponentTypeId id = componentTypeId<T>();
  for (Component* component = m_firstComponent;
       nullptr != component;
       component = component->getNextComponent()) {
    if (component->getTypeId() == id) {
      return static_cast<T*>(component);
    }
  }
  return nullptr;
}

template<typename T>
void
SceneNode::removeComponent() {
  const ComponentTypeId id = componentTypeId<T>();
  for (Component* component = m_firstComponent;
       nullptr != component;
       component = component->getNextComponent()) {
    
    if (component->getTypeId() == id) {
      unlinkComponent(component);
      MemoryPool<T>& pool = MemoryPoolHandler::instance().pool<T>();
      pool.deallocate(id, static_cast<void*>(component));
      return;
    }
  }
}

}  // namespace sfmx

// SceneNode is pooled like any other type by MemoryPoolHandler, so it needs a
// type id to key its pool (see registerPool<SceneNode>).
DECLARE_TYPE_TRAITS(sfmx::SceneNode)
