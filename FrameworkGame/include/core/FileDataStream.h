#pragma once

#include <fstream>

#include "core/DataStream.h"

namespace sfmx
{

/**
 * @brief @ref DataStream backed by a file on disk.
 *
 * Open mode follows @ref AccessMode: read-only (`in`), write-only truncating
 * (`out|trunc`), or **read+write** (`in|out`, preserving content; add
 * `kTruncate` to start empty). Read+write supports editing a large file in
 * place without buffering it all in memory.
 *
 * std::fstream requires a reposition between an input and a subsequent output
 * (and vice versa); this class satisfies that automatically by re-seeking to its
 * authoritative @ref m_pos on each direction switch, so callers never have to.
 *
 * Construction never throws: if the file cannot be opened, @ref isOpen returns
 * false (factories such as @ref FileSystem::openFile return nullptr in that
 * case). Note `in|out` (read+write without `kTruncate`) requires the file to
 * already exist.
 */
class SFMX_UTILITY_EXPORT FileDataStream : public DataStream
{
 public:
  /**
   * @brief Open @p path. @p mode containing kWrite opens for writing
   *        (truncating); otherwise opens for reading.
   */
  explicit FileDataStream(const FileSystemPath& path,
                          AccessModeFlags mode = AccessMode::kRead);

  ~FileDataStream() override;

  /** @brief True if the underlying file is open and usable. */
  NODISCARD FORCEINLINE bool
  isOpen() const { return m_stream.is_open(); }

  /** @brief Path this stream was opened from. */
  NODISCARD FORCEINLINE const FileSystemPath&
  getPath() const { return m_path; }

  NODISCARD bool
  isFile() const override { return true; }

  size_t
  read(void* dst, size_t bytes) override;

  size_t
  write(const void* src, size_t bytes) override;

  void
  seek(size_t pos) override;

  void
  skip(int64 count) override;

  NODISCARD size_t
  tell() const override;

  NODISCARD bool
  isAtEnd() const override;

  void
  close() override;

  NODISCARD SPtr<DataStream>
  clone() const override;

 private:
  // Tracks the last data op so a read+write stream can insert the reposition
  // std::fstream requires when switching direction.
  enum class LastOp { kNone, kRead, kWrite };

  FileSystemPath m_path;
  std::fstream m_stream;
  // Authoritative cursor, kept in lockstep with m_stream by every op. Drives
  // size tracking and tell()/isAtEnd() without querying tellp()/tellg().
  size_t m_pos = 0;
  bool m_readWrite = false;       // opened in|out (both kRead and kWrite)
  LastOp m_lastOp = LastOp::kNone;
};

} // namespace sfmx
