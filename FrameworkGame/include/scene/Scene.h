#pragma once

#include <type_traits>
#include <utility>

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "core/MemoryPool.h"
#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "scene/SceneNode.h"
#include "scene/SceneTypes.h"
#include "utils/MemoryPoolHandler.h"

namespace sfmx
{

/**
 * @brief Owns a scene hierarchy: its ids, the node registry, and traversal.
 *
 * The Scene allocates the stable @ref NodeId handles, keeps a NodeId->node
 * registry for O(1) lookup, and drives the per-frame update/draw traversal from
 * the root. All node creation and destruction funnels through the Scene so ids,
 * registry entries, and pooled storage stay consistent.
 *
 * The Scene does not own any storage: both the SceneNode pool and every
 * component pool live in the process-wide @ref MemoryPoolHandler. Those pools
 * (in particular the SceneNode pool) must be registered on the handler before
 * any Scene is constructed.
 */
class Scene
{
 public:
  explicit Scene(StringView name);
  ~Scene();

  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;

  NODISCARD SceneNode* getRoot() const { return m_root; }
  NODISCARD const char* getName() const { return m_name.data(); }

  /**
   * @brief Create a node under @p parent (the root when null) and register it.
   * @return The new node, or nullptr if the node pool is exhausted.
   */
  SceneNode* createNode(StringView name, SceneNode* parent = nullptr);

  /** @brief Destroy @p node and its whole subtree, freeing pool slots. */
  void destroyNode(SceneNode* node);
  /** @brief Destroy the node with id @p id (if any) and its subtree. */
  void destroyNode(NodeId id);

  /** @brief Resolve a handle to a live node, or nullptr if dead/unknown. */
  NODISCARD SceneNode* findNode(NodeId id) const;

  /** @brief Every node whose name equals @p name (names are not unique). */
  NODISCARD Vector<SceneNode*> findNodesByName(StringView name) const;

  NODISCARD size_t getNodeCount() const { return m_registry.size(); }

  void update(float deltaTime);
  void draw(sf::RenderTarget& target) const;

 private:
  NodeId allocateId() { return m_nextId++; }
  void registerNode(SceneNode* node);
  void unregisterNode(NodeId id);
  void destroyNodeRecursive(SceneNode* node);

  Array<char, kMaxNameLength> m_name;
  SceneNode* m_root;
  NodeId m_nextId;
  UnorderedMap<NodeId, SceneNode*> m_registry;
};

// ---------------------------------------------------------------------------
// SceneNode component templates: defined here, where MemoryPoolHandler (and so
// the typed pools the components are stored in) is a complete type.
// ---------------------------------------------------------------------------

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
void
SceneNode::removeComponent() {
  const ComponentTypeId id = componentTypeId<T>();
  for (Component* component = m_firstComponent;
       nullptr != component;
       component = component->getNextComponent()) {
    if (component->getTypeId() == id) {
      unlinkComponent(component);
      MemoryPool<T>& pool = MemoryPoolHandler::instance().pool<T>();
      pool.deallocate(static_cast<T*>(component));
      return;
    }
  }
}

}  // namespace sfmx
