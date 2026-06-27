#pragma once

#include "core/platform/Prerequisites.h"
#include "utils/Module.h"

namespace sf
{
class RenderTarget;
}

namespace sfmx
{

class Scene;

/**
 * @brief Owns every @ref Scene and drives the create -> load -> unload -> destroy
 *        lifecycle, while tracking which scenes are currently active.
 *
 * A @ref Module (singleton), reached through @c SceneManager::instance() once
 * @c startUp has run. Each scene is owned here, keyed by a unique name. Several
 * scenes can be active at once (the @em active @em set, iterated in insertion
 * order for update/draw); one of them is the @em current scene returned by
 * @ref getActiveScene.
 *
 * Two complementary meanings of load/unload are supported:
 *  - @ref createScene allocates a new empty scene and activates it.
 *  - @ref loadScene with a path deserializes a @c .sfmxasset from disk (via
 *    @ref SceneSerializer) and activates it; @ref loadScene with only a name
 *    re-activates an already-created scene.
 *  - @ref unloadScene deactivates a scene but keeps it alive and owned.
 *  - @ref destroyScene frees a scene entirely.
 *
 * Lifecycle ordering: start the manager AFTER @ref MemoryPoolHandler and its
 * SceneNode / component pools are registered (scene construction allocates the
 * root node from the pool), and shut it down BEFORE the pools are torn down so
 * that @ref Scene::clear can return pooled storage while the pools still live.
 */
class SFMX_UTILITY_EXPORT SceneManager : public Module<SceneManager>
{
 public:
  /**
   * @brief Allocate a new empty scene named @p name and activate it.
   * @return The new scene, or @c nullptr if @p name is already taken.
   */
  NODISCARD Scene*
  createScene(StringView name);

  /**
   * @brief Activate an already-created scene (add it to the active set).
   * @return The scene, or @c nullptr if no scene is named @p name. Idempotent.
   */
  Scene*
  loadScene(StringView name);

  /**
   * @brief Create a scene named @p name and rebuild it from the @c .sfmxasset at
   *        @p path, then activate it.
   * @return The loaded scene, or @c nullptr if @p name is taken or the file
   *         could not be read (no scene is left behind on failure).
   */
  NODISCARD Scene*
  loadScene(StringView name, const FileSystemPath& path);

  /** @brief Write the scene named @p name to @p path as a @c .sfmxasset. */
  NODISCARD bool
  saveScene(StringView name, const FileSystemPath& path) const;

  /** @brief Deactivate the scene named @p name; it stays alive and owned. */
  void
  unloadScene(StringView name);

  /** @brief Deactivate @p scene; it stays alive and owned. */
  void
  unloadScene(Scene* scene);

  /** @brief Deactivate and free the scene named @p name. */
  void
  destroyScene(StringView name);

  /** @brief Deactivate and free @p scene. */
  void
  destroyScene(Scene* scene);

  /** @brief Deactivate and free every scene. */
  void
  destroyAllScenes();

  /** @brief The current active scene, or @c nullptr if none is active. */
  NODISCARD Scene*
  getActiveScene() const { return m_activeScene; }

  /** @brief Make the scene named @p name current, activating it if needed. */
  void
  setActiveScene(StringView name);

  /** @brief Make @p scene current, activating it if needed. */
  void
  setActiveScene(Scene* scene);

  /** @brief Every active scene, in update/draw order. */
  NODISCARD const Vector<Scene*>&
  getActiveScenes() const { return m_activeScenes; }

  /** @brief The owned scene named @p name (active or not), or @c nullptr. */
  NODISCARD Scene*
  findScene(StringView name) const;

  /** @brief Whether a scene named @p name is owned (active or not). */
  NODISCARD bool
  hasScene(StringView name) const;

  /** @brief Whether @p scene is currently in the active set. */
  NODISCARD bool
  isActive(const Scene* scene) const;

  /** @brief Number of owned scenes (active and inactive). */
  NODISCARD size_t
  getSceneCount() const { return m_scenes.size(); }

  /** @brief Update every active scene, in order, by @p deltaTime seconds. */
  void
  update(float deltaTime);

  /** @brief Draw every active scene, in order, to @p target. */
  void
  draw(sf::RenderTarget& target) const;

 protected:
  // Destroy all scenes while the MemoryPoolHandler pools are still alive (same
  // ordering rule as the other modules: shut down before the pools).
  void
  onShutDown() override;

 private:
  friend class Module<SceneManager>;
  
  SceneManager() = default;

  // Removes scene from the active set, promoting a new current scene if it was
  // the current one. Does not free the scene.
  void
  deactivate(Scene* scene);

  UnorderedMap<String, UniquePtr<Scene>> m_scenes;
  Vector<Scene*> m_activeScenes;
  Scene* m_activeScene = nullptr;
};

} // namespace sfmx
