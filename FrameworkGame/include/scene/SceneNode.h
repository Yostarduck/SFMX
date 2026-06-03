#pragma once

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "scene/SceneTypes.h"
#include "scene/Transform.h"

namespace sfmx
{
class Scene;

/**
 * @brief One node in the scene hierarchy.
 *
 * A node carries a stable @ref NodeId, a fixed-length name, enable/visible
 * flags, an inline @ref Transform, a list of attached @ref Component "components",
 * and links to its parent and children. Nodes are owned by a Scene's memory
 * pool and never relocate, so the parent/child and component links are plain
 * non-owning pointers into that pooled storage.
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
  NODISCARD const Vector<SceneNode*>& getChildren() const { return m_children; }
  NODISCARD size_t getChildCount() const { return m_children.size(); }

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

  NodeId m_id;
  Array<char, kMaxNameLength> m_name;
  bool m_enabled;
  bool m_visible;
  SceneNode* m_parent;
  Vector<SceneNode*> m_children;
  Scene* m_scene;
  Transform m_transform;
  Vector<Component*> m_components;
};

// -- Inline template that needs only Component (not Scene) --------------------
template<typename T>
NODISCARD T*
SceneNode::getComponent() const {
  const ComponentTypeId id = componentTypeId<T>();
  for (Component* component : m_components) {
    if (component->getTypeId() == id) {
      return static_cast<T*>(component);
    }
  }
  return nullptr;
}

}  // namespace sfmx
