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

namespace sfmx
{

/**
 * @brief Type-erased handle to a component pool, so the Scene can return any
 *        component to the correct typed pool without RTTI.
 */
class IComponentPool
{
 public:
  virtual ~IComponentPool() = default;
  virtual void deallocate(Component* component) = 0;
};

/**
 * @brief A pool of components of one concrete type @p T.
 */
template<typename T>
class ComponentPool : public IComponentPool
{
 public:
  explicit ComponentPool(size_t capacity) { m_pool.initialize(capacity); }

  NODISCARD MemoryPool<T>& pool() { return m_pool; }

  void
  deallocate(Component* component) override {
    m_pool.deallocate(static_cast<T*>(component));
  }

 private:
  MemoryPool<T> m_pool;
};

/**
 * @brief Owns a scene hierarchy and all of its storage.
 *
 * The Scene owns one large MemoryPool for every SceneNode plus one pool per
 * concrete component type, allocates the stable @ref NodeId handles, keeps a
 * NodeId->node registry for O(1) lookup, and drives the per-frame update/draw
 * traversal from the root. All node creation and destruction funnels through
 * the Scene so ids, registry entries, and pooled storage stay consistent.
 *
 * Node and component pools are fixed-size: size the node pool (constructor) and
 * each component pool (@ref registerComponent) to comfortably exceed demand.
 */
class Scene
{
 public:
  Scene(StringView name, size_t maxNodes);
  ~Scene();

  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;

  /**
   * @brief Create the pool for component type @p T with its own @p capacity.
   *
   * Call once per component type during setup, before attaching components of
   * that type. Frequent components (e.g. sprites) warrant large pools; rare
   * ones (e.g. cameras) small ones.
   */
  template<typename T>
  void registerComponent(size_t capacity);

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

  /**
   * @brief Access the pool for component type @p T (used by SceneNode).
   *
   * Lazily creates the pool at the default capacity if @ref registerComponent
   * was never called for @p T, asserting in debug so the omission is caught.
   */
  template<typename T>
  NODISCARD ComponentPool<T>& componentPool();

 private:
  NodeId allocateId() { return m_nextId++; }
  void registerNode(SceneNode* node);
  void unregisterNode(NodeId id);
  void destroyNodeRecursive(SceneNode* node);

  Array<char, kMaxNameLength> m_name;
  MemoryPool<SceneNode> m_nodePool;
  UnorderedMap<ComponentTypeId, UniquePtr<IComponentPool>> m_componentPools;
  size_t m_defaultComponentCapacity;
  SceneNode* m_root;
  NodeId m_nextId;
  UnorderedMap<NodeId, SceneNode*> m_registry;
};

// ---------------------------------------------------------------------------
// Template definitions that need the complete Scene / ComponentPool types.
// ---------------------------------------------------------------------------

template<typename T>
void
Scene::registerComponent(size_t capacity) {
  static_assert(std::is_base_of<Component, T>::value,
                "registerComponent<T>: T must derive from Component");
  const ComponentTypeId id = componentTypeId<T>();
  SFMX_ASSERT(m_componentPools.find(id) == m_componentPools.end());
  m_componentPools[id] = UniquePtr<IComponentPool>(new ComponentPool<T>(capacity));
}

template<typename T>
NODISCARD ComponentPool<T>&
Scene::componentPool() {
  static_assert(std::is_base_of<Component, T>::value,
                "componentPool<T>: T must derive from Component");
  const ComponentTypeId id = componentTypeId<T>();
  const auto it = m_componentPools.find(id);
  if (it != m_componentPools.end()) {
    return *static_cast<ComponentPool<T>*>(it->second.get());
  }

  // Not registered: create it at the default capacity, but assert so a missing
  // registerComponent<T>() shows up in debug builds.
  SFMX_ASSERT(false && "component type used without registerComponent<T>()");
  ComponentPool<T>* created = new ComponentPool<T>(m_defaultComponentCapacity);
  m_componentPools[id] = UniquePtr<IComponentPool>(created);
  return *created;
}

template<typename T, typename... Args>
T*
SceneNode::addComponent(Args&&... args) {
  static_assert(std::is_base_of<Component, T>::value,
                "addComponent<T>: T must derive from Component");
  MemoryPool<T>& pool = m_scene->componentPool<T>().pool();
  T* component = pool.allocate(this, std::forward<Args>(args)...);
  if (nullptr != component) {
    m_components.push_back(component);
  }
  return component;
}

template<typename T>
void
SceneNode::removeComponent() {
  const ComponentTypeId id = componentTypeId<T>();
  for (size_t i = 0; i < m_components.size(); ++i) {
    if (m_components[i]->getTypeId() == id) {
      m_scene->componentPool<T>().pool().deallocate(static_cast<T*>(m_components[i]));
      m_components.erase(m_components.begin() + static_cast<ptrdiff_t>(i));
      return;
    }
  }
}

}  // namespace sfmx
