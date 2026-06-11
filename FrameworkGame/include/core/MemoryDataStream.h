#pragma once

#include "core/DataStream.h"

namespace sfmx
{

/**
 * @brief In-memory @ref DataStream backed by a growable byte buffer.
 *
 * Read+write. Writing past the current end grows the buffer (so you don't need
 * to know the final size up front), which makes it the natural target for
 * "serialize now, decide size later, then dump to disk".
 */
class SFMX_UTILITY_EXPORT MemoryDataStream : public DataStream
{
 public:
  /** @brief Empty, growable stream. */
  MemoryDataStream();

  /** @brief Copy @p size bytes from @p data into the buffer; cursor at start. */
  MemoryDataStream(const void* data, size_t size);

  /** @brief Pre-buffer: read @p source fully into memory; cursor at start. */
  explicit MemoryDataStream(const SPtr<DataStream>& source);

  ~MemoryDataStream() override;

  NODISCARD bool
  isFile() const override { return false; }

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

  /** @brief Pointer to the start of the buffer (for dumping to disk). */
  NODISCARD FORCEINLINE const uint8*
  data() const { return m_data.data(); }

 private:
  Vector<uint8> m_data;
  size_t m_pos = 0;
};

} // namespace sfmx
