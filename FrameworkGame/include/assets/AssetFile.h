#pragma once

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"
#include "assets/AssetMetadata.h"

namespace sfmx
{

/** @brief Magic at the start of every `.sfmxasset` ("SFMX", not terminated). */
constexpr ansichar kAssetMagic[4] = {'S', 'F', 'M', 'X'};
/** @brief Container format version (bump on layout changes). */
constexpr uint32 kAssetFormatVersion = 1;

/**
 * @brief One entry in the chunk directory.
 *
 * @c size is the bytes stored on disk (compressed size when @c compression is
 * not @ref ChunkCompression::kNone); @c rawSize is the decompressed size
 * (== @c size when uncompressed).
 */
struct ChunkEntry {
  uint32           id          = 0;
  uint64           offset      = 0;   // absolute byte offset of the payload in the file
  uint64           size        = 0;   // bytes on disk
  uint64           rawSize     = 0;   // bytes after decompression
  ChunkFormat      format      = ChunkFormat::kRaw;
  ChunkCompression compression = ChunkCompression::kNone;
};

/**
 * @brief Builds a `.sfmxasset` and writes it to a @ref DataStream.
 *
 * Build-time / cook-time use (heap allocation is fine here): set the metadata,
 * add references and chunks, then @ref writeTo. Chunk bytes are buffered until
 * @ref writeTo lays out the offsets.
 */
class SFMX_UTILITY_EXPORT AssetFileWriter
{
 public:
  void
  setMetadata(const AssetMetadata& meta) { m_metadata = meta; }

  /** @brief Record a dependency UUID (stored in the cheap-scan region). */
  void
  addReference(const UUID& dependency);

  /**
   * @brief Append a chunk; its bytes are copied into the writer.
   * @return The chunk's index.
   */
  uint32
  addChunk(const void* data,
           size_t size,
           ChunkFormat format = ChunkFormat::kRaw,
           ChunkCompression compression = ChunkCompression::kNone);

  /** @brief Serialize header + metadata + references + directory + chunks. */
  bool
  writeTo(DataStream& out) const;

 private:
  struct PendingChunk {
    Vector<uint8>    data;
    ChunkFormat      format      = ChunkFormat::kRaw;
    ChunkCompression compression = ChunkCompression::kNone;
  };

  AssetMetadata        m_metadata;
  Vector<UUID>         m_references;
  Vector<PendingChunk> m_chunks;
};

/**
 * @brief Opens a `.sfmxasset`: reads the scan region (header + metadata +
 *        references) cheaply, and reads chunk payloads on demand.
 *
 * Holds the stream so @ref readChunk can seek straight to a chunk without ever
 * loading the whole payload — the basis for lazy/streamed loading later.
 */
class SFMX_UTILITY_EXPORT AssetFileReader
{
 public:
  /** @brief Read header + metadata + references. Does NOT read any payload. */
  bool
  open(const SPtr<DataStream>& stream);

  NODISCARD FORCEINLINE bool
  isOpen() const { return nullptr != m_stream; }

  NODISCARD FORCEINLINE const AssetMetadata&
  metadata() const { return m_metadata; }

  NODISCARD FORCEINLINE const Vector<UUID>&
  references() const { return m_references; }

  NODISCARD FORCEINLINE size_t
  chunkCount() const { return m_chunks.size(); }

  NODISCARD FORCEINLINE const ChunkEntry&
  chunk(size_t index) const { return m_chunks[index]; }

  /** @brief Read chunk @p index's raw on-disk bytes into @p out (seeks to it). */
  bool
  readChunk(size_t index, Vector<uint8>& out) const;

  /**
   * @brief Release the underlying stream (and the file handle it holds).
   *
   * Already-read metadata / references / chunk directory stay queryable (they
   * live in memory); only @ref readChunk needs the stream and will fail after.
   */
  void
  close() { m_stream.reset(); }

 private:
  SPtr<DataStream>   m_stream;
  AssetMetadata      m_metadata;
  Vector<UUID>       m_references;
  Vector<ChunkEntry> m_chunks;
};

} // namespace sfmx
