#pragma once

#include "core/platform/Prerequisites.h"
#include "utils/Flags.h"

namespace sfmx
{

/**
 * @brief How a @ref DataStream may be accessed. Combine via @ref AccessModeFlags.
 */
enum class AccessMode : uint16 {
  kNone  = 0x00,
  kRead  = 0x01,
  kWrite = 0x02,
};
SFMX_DECLARE_FLAGS_EXT(AccessModeFlags, AccessMode, uint16)

/**
 * @brief Abstract binary stream: a uniform read/write/seek surface over some
 *        backing store (memory, file, ...).
 *
 * Serialization code writes against a `DataStream&` and never needs to know the
 * backing store, so the same code can target memory or a file, and the store
 * can be swapped (build in memory, then dump to disk).
 *
 * The base supplies ergonomic helpers on top of the raw @ref read / @ref write:
 * - `operator<<` / `operator>>` move **trivially-copyable** values as raw bytes
 *   (a `static_assert` rejects anything else — no silent serialization of
 *   pointers/`String`/containers).
 * - @ref writeString / @ref readString move a length-prefixed UTF-8 string, the
 *   round-trippable form for strings embedded in a binary blob.
 *
 * Endianness: v1 assumes the host endianness (all target platforms are
 * little-endian); raw POD writes are not byte-swapped.
 */
class SFMX_UTILITY_EXPORT DataStream
{
 public:
  explicit DataStream(AccessModeFlags mode = AccessMode::kRead)
    : m_mode(mode)
  {}

  virtual ~DataStream() = default;

  DataStream(const DataStream&) = delete;
  DataStream& operator=(const DataStream&) = delete;

  NODISCARD FORCEINLINE bool
  isReadable() const { return m_mode.isSetAny(AccessMode::kRead); }

  NODISCARD FORCEINLINE bool
  isWriteable() const { return m_mode.isSetAny(AccessMode::kWrite); }

  /** @brief Total size of the backing store in bytes. */
  NODISCARD FORCEINLINE size_t
  size() const { return m_size; }

  // -- Raw byte interface (implemented per backing store) --------------------
  /** @brief True if this stream is backed by a file. */
  NODISCARD virtual bool
  isFile() const = 0;

  /** @brief Read up to @p bytes into @p dst; returns the count actually read. */
  virtual size_t
  read(void* dst, size_t bytes) = 0;

  /** @brief Write @p bytes from @p src; returns the count actually written. */
  virtual size_t
  write(const void* src, size_t bytes) = 0;

  /** @brief Move the cursor to absolute byte offset @p pos. */
  virtual void
  seek(size_t pos) = 0;

  /** @brief Move the cursor by @p count bytes (negative rewinds). */
  virtual void
  skip(int64 count) = 0;

  /** @brief Current cursor offset from the start, in bytes. */
  NODISCARD virtual size_t
  tell() const = 0;

  /** @brief True once the cursor has reached the end. */
  NODISCARD virtual bool
  isAtEnd() const = 0;

  /** @brief Release the backing store; further operations are invalid. */
  virtual void
  close() = 0;

  /** @brief Independent copy of this stream (cursor reset to the start). */
  NODISCARD virtual SPtr<DataStream>
  clone() const = 0;

  // -- Typed helpers (built on read/write) -----------------------------------
  /** @brief Write @p value as raw bytes. Trivially-copyable types only. */
  template<typename T>
  FORCEINLINE DataStream&
  operator<<(const T& value) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "operator<< serializes raw bytes; for String/containers/types "
                  "with pointers use writeString() or an explicit helper.");
    SFMX_ASSERT(isWriteable());
    write(&value, sizeof(T));
    return *this;
  }

  /** @brief Read @p value from raw bytes. Trivially-copyable types only. */
  template<typename T>
  FORCEINLINE DataStream&
  operator>>(T& value) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "operator>> deserializes raw bytes; for String/containers/types "
                  "with pointers use readString() or an explicit helper.");
    SFMX_ASSERT(isReadable());
    read(&value, sizeof(T));
    return *this;
  }

  /** @brief Write a length-prefixed UTF-8 string: [uint64 len][bytes]. */
  void
  writeString(StringView str);

  /** @brief Read a string previously written with @ref writeString. */
  NODISCARD String
  readString();

 protected:
  size_t m_size = 0;
  AccessModeFlags m_mode;
};

} // namespace sfmx
