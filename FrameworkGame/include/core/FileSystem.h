#pragma once

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"

namespace sfmx
{

/**
 * @brief Thin platform filesystem facade and the single place that touches
 *        std::filesystem.
 *
 * Keeping every std::filesystem call behind this class means a future change of
 * backend is contained here. It supplies the stream factories the rest of the
 * engine uses (so callers get a @ref DataStream, not a raw file handle), a
 * memory->disk dump helper, whole-file read, and directory iteration (used by
 * the asset scan).
 *
 * All methods are static; the one-line query/op wrappers never throw (they use
 * the std::error_code overloads and report success as a bool).
 */
class SFMX_UTILITY_EXPORT FileSystem
{
 public:
  // -- Stream factories ------------------------------------------------------
  /**
   * @brief Open @p path as a stream. @p mode with kWrite opens for writing
   *        (truncating). Returns nullptr if the file cannot be opened.
   */
  NODISCARD static SPtr<DataStream>
  openFile(const FileSystemPath& path, AccessModeFlags mode = AccessMode::kRead);

  /**
   * @brief Create @p path's parent directories (if needed) and open it for
   *        writing. Returns nullptr on failure.
   */
  NODISCARD static SPtr<DataStream>
  createAndOpenFile(const FileSystemPath& path);

  /**
   * @brief Write @p source in full (from its start) into @p path, creating
   *        parent directories. Works with any stream backing. Returns success.
   */
  static bool
  dumpMemStreamIntoFile(const SPtr<DataStream>& source, const FileSystemPath& path);

  /** @brief Read an entire file into a byte vector (empty on failure). */
  NODISCARD static Vector<uint8>
  fastRead(const FileSystemPath& path);

  // -- Queries / operations (std::filesystem wrappers, noexcept) -------------
  NODISCARD static bool
  exists(const FileSystemPath& path);

  NODISCARD static bool
  isFile(const FileSystemPath& path);

  NODISCARD static bool
  isDirectory(const FileSystemPath& path);

  /** @brief Create @p path and any missing parents. True if it now exists. */
  static bool
  createDirectories(const FileSystemPath& path);

  /** @brief Remove a file or empty directory. */
  static bool
  remove(const FileSystemPath& path);

  /** @brief Remove a path and everything under it. */
  static bool
  removeAll(const FileSystemPath& path);

  static bool
  rename(const FileSystemPath& from, const FileSystemPath& to);

  /** @brief OS temporary directory. */
  NODISCARD static FileSystemPath
  tempDirectory();

  // -- Iteration -------------------------------------------------------------
  /** @brief Invoke @p fn for each immediate child of @p path. */
  static void
  forEachFileChild(const FileSystemPath& path,
                   const Function<void(const FileSystemPath&)>& fn);

  /** @brief Invoke @p fn for each descendant of @p path (recursively). */
  static void
  forEachFileChildRecursive(const FileSystemPath& path,
                            const Function<void(const FileSystemPath&)>& fn);
};

} // namespace sfmx
