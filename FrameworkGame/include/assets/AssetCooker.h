#pragma once

#include "core/platform/Prerequisites.h"

namespace sfmx
{

/** @brief Tally returned by @ref AssetCooker::cookDirectory. */
struct CookStats
{
  size_t cooked  = 0;   //!< Source files wrapped into a `.sfmxasset`.
  size_t skipped = 0;   //!< Files skipped (unsupported extension or IO failure).
};

/**
 * @brief Offline cooker: wraps external media (PNG/JPG, OGG/WAV/FLAC) into the
 *        engine-native `.sfmxasset` container, ready for the @ref AssetManager.
 *
 * Cooking only copies the source bytes into a tagged chunk plus metadata — it
 * never decodes the payload and never touches the GPU, so it runs headless. Each
 * asset gets a deterministic UUID derived from its path relative to the source
 * root (@c UUID::createFromName), so references resolve identically across cooks.
 *
 * This is a build-time tool; heap allocation here is fine (it is not the game loop).
 */
class SFMX_UTILITY_EXPORT AssetCooker
{
 public:
  /**
   * @brief Cook one @p source file into @c <outputDir>/<rel>.sfmxasset, where
   *        @c rel is @p source relative to @p sourceRoot.
   * @return True on success; false (counts as skipped) for an unsupported
   *         extension or an IO failure.
   */
  static bool
  cookFile(const FileSystemPath& source,
           const FileSystemPath& sourceRoot,
           const FileSystemPath& outputDir);

  /**
   * @brief Recursively cook every supported file under @p sourceDir into
   *        @p outputDir (mirroring the relative subtree).
   * @return Counts of cooked vs skipped files.
   */
  static CookStats
  cookDirectory(const FileSystemPath& sourceDir,
                const FileSystemPath& outputDir);
};

} // namespace sfmx
