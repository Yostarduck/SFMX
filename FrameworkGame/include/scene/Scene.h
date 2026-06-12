#pragma once

#include <type_traits>
#include <utility>

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/CameraComponent.h"
#include "scene/Component.h"
#include "scene/SceneNode.h"
#include "scene/SceneTypes.h"

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

  // -- Cameras --------------------------------------------------------------
  /** @brief Convenience: clears all cameras and activates a single one. */
  void setCamera(CameraComponent* camera);
  /** @brief Returns the first camera in the list, or nullptr if empty. */
  NODISCARD CameraComponent* getCamera() const;
  /** @brief Appends a camera to the draw list. */
  void addCamera(CameraComponent* camera);
  /** @brief Removes a specific camera from the list. */
  void removeCamera(const CameraComponent* camera);
  /** @brief Removes all cameras. */
  void clearCameras();
  /** @brief Number of registered cameras. */
  NODISCARD size_t getCameraCount() const { return m_cameras.size(); }

  void update(float deltaTime);
  void draw(sf::RenderTarget& target) const;

  /** @brief Destroy every node and component owned by this scene, keeping the
   * root alive until pool shutdown. */
  void clear();

 private:
  NodeId allocateId() { return m_nextId++; }
  void registerNode(SceneNode* node);
  void unregisterNode(NodeId id);
  void destroyNodeRecursive(SceneNode* node);

  Array<char, kMaxNameLength> m_name;
  SceneNode* m_root;
  Vector<CameraComponent*> m_cameras;
  NodeId m_nextId;
  UnorderedMap<NodeId, SceneNode*> m_registry;
};

}  // namespace sfmx
