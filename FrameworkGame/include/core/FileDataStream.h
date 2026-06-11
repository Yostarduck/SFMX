#pragma once

#include <fstream>

#include "core/DataStream.h"

namespace sfmx
{

/**
 * @brief @ref DataStream backed by a file on disk.
 *
 * A stream is opened for **reading or writing**, not both (write opens with
 * truncation). This keeps the std::fstream get/put-pointer handling simple and
 * unambiguous and covers the two real uses: read a cooked asset, or dump a
 * built buffer to disk.
 *
 * Construction never throws: if the file cannot be opened, @ref isOpen returns
 * false (factories such as @ref FileSystem::openFile return nullptr in that
 * case).
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
  FileSystemPath m_path;
  // mutable: tell()/isAtEnd() are const but std::fstream queries are not.
  mutable std::fstream m_stream;
};

} // namespace sfmx
