/************************************************************************/
/**
 * @file UISerializer.h
 * @author Swampertor
 * @date 2026/09/23
 * @brief  Saves/loads UI widget trees ("UI documents") as binary.
 */
/************************************************************************/
#pragma once

#include "core/platform/Prerequisites.h"
#include "core/Path.h"

namespace sfmx
{

class DataStream;
class UIWidget;

/**
 * @brief Saves/loads UI widget trees, to/from memory and to `.sfmxasset`.
 *
 * Format (mirrors @ref SceneSerializer): the container is a single-chunk
 * `.sfmxasset` (raw payload, LZ4-compressed on write, inflated transparently
 * on read). Inside the blob every widget is a record
 *
 *     [type UUID][payload byte size][child count][payload]  + children, depth-first
 *
 * so an unknown type id costs only its own bytes — the size prefix and child
 * count keep the stream aligned exactly like the scene's component records.
 * The payload is @ref UIWidget::onSerialize output (widget version byte +
 * @ref UIWidget::serializeBase shared state + type-specific fields).
 *
 * Stateless utility (all static), like @ref SceneSerializer. Loading requires
 * @ref UIWidgetRegistry started with the types registered and their pools
 * registered — @ref UIManager::onStartUp does both for the built-ins.
 * Returned roots are created but attached nowhere; the caller roots them
 * (see @ref UIManager::loadUI).
 */
class SFMX_UTILITY_EXPORT UISerializer
{
 public:
  /** @brief Write @p roots (and their subtrees) to @p out, depth-first. */
  NODISCARD static bool
  serialize(const Vector<UIWidget*>& roots, DataStream& out);

  /**
   * @brief Rebuild a tree from @p in.
   *
   * @param outRoots Cleared, then filled with the reconstructed roots.
   * Records of unknown type are skipped cleanly — their bytes are consumed
   * (including each child's subtree) but nothing is created.
   */
  NODISCARD static bool
  deserialize(Vector<UIWidget*>& outRoots, DataStream& in);

  /** @brief Write a `.sfmxasset` whose single raw chunk is the UI document. */
  NODISCARD static bool
  saveToFile(const Vector<UIWidget*>& roots, const FileSystemPath& path);

  /** @brief Read a `.sfmxasset` document into @p outRoots. */
  NODISCARD static bool
  loadFromFile(Vector<UIWidget*>& outRoots, const FileSystemPath& path);
};

} // namespace sfmx
