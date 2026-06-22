#pragma once

#include "core/platform/Prerequisites.h"

namespace sfmx
{

class Scene;
class DataStream;

/**
 * @brief Serializes a @ref Scene graph to/from binary, and to/from `.sfmxasset`.
 *
 * Walks the hierarchy depth-first (parent before child) writing each node's id,
 * parent id, name, flags, transform, and its components (each via
 * @ref Component::onSerialize, framed by type id + byte size). Rebuilding clears
 * the target scene and recreates nodes with @ref Scene::createNode and components
 * with @ref ComponentRegistry::create, then drives @ref Component::onDeserialize.
 *
 * Stateless utility (all static), like @ref FileSystem. Deserialization requires
 * @ref ComponentRegistry to be started with the relevant component types
 * registered, and their pools registered — same preconditions as @c addComponent.
 *
 * The scene root is structural and is NOT serialized (its descendants are);
 * components attached directly to the root are not persisted.
 */
class SFMX_UTILITY_EXPORT SceneSerializer
{
 public:
  /** @brief Write @p scene's graph (descendants of root) to @p out. */
  NODISCARD static bool
  serialize(const Scene& scene, DataStream& out);

  /**
   * @brief Clear @p scene and rebuild it from @p in.
   *
   * Uses @ref ComponentRegistry::instance to create components by type id;
   * components whose type is not registered are skipped cleanly (their bytes are
   * consumed but no component is attached).
   */
  NODISCARD static bool
  deserialize(Scene& scene, DataStream& in);

  /** @brief Write a `.sfmxasset` whose single raw chunk is the serialized scene. */
  NODISCARD static bool
  saveToFile(const Scene& scene, const FileSystemPath& path);

  /** @brief Read a `.sfmxasset` produced by @ref saveToFile into @p scene. */
  NODISCARD static bool
  loadFromFile(Scene& scene, const FileSystemPath& path);
};

} // namespace sfmx
