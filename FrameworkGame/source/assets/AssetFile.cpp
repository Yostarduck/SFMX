#include "assets/AssetFile.h"

#include <cstring>

#include "core/DataStreamTypes.h"  // UUID operator<< / >>

namespace sfmx
{

namespace
{

// On-disk sizes of the fixed regions (serialized field-by-field, not blitted —
// so they're summed per field by type, never sizeof(struct) which would include
// padding).
constexpr uint64 kHeaderBytes =
    sizeof(kAssetMagic) +          // magic "SFMX"
    sizeof(kAssetFormatVersion) +  // format version (uint32)
    sizeof(uint64) +               // metadataOffset
    sizeof(uint64) +               // referencesOffset
    sizeof(uint64) +               // directoryOffset
    sizeof(uint32);                // chunkCount

constexpr uint64 kChunkEntryBytes =
    sizeof(uint32) +  // id
    sizeof(uint64) +  // offset
    sizeof(uint64) +  // size
    sizeof(uint64) +  // rawSize
    sizeof(uint16) +  // format      (ChunkFormat written as uint16)
    sizeof(uint16);   // compression (ChunkCompression written as uint16)

void
writeMetadata(DataStream& s, const AssetMetadata& m) {
  s << m.uuid;
  s << m.assetType;
  s << m.creationTime;
  s << m.version;
  s.write(m.typeName, kAssetTypeNameLength);
  s.write(m.name, kAssetNameLength);
  s.write(m.sourcePath, kAssetSourcePathLength);
}

void
readMetadata(DataStream& s, AssetMetadata& m) {
  s >> m.uuid;
  s >> m.assetType;
  s >> m.creationTime;
  s >> m.version;
  s.read(m.typeName, kAssetTypeNameLength);
  s.read(m.name, kAssetNameLength);
  s.read(m.sourcePath, kAssetSourcePathLength);
  // Defend against a corrupt/non-terminated file: keep the char arrays safe to
  // hand to String().
  m.typeName[kAssetTypeNameLength - 1]     = '\0';
  m.name[kAssetNameLength - 1]             = '\0';
  m.sourcePath[kAssetSourcePathLength - 1] = '\0';
}

// True if a [offset, offset+len) span fits within a file of `total` bytes,
// without overflowing.
FORCEINLINE bool
spanFits(uint64 offset, uint64 len, uint64 total) {
  return offset <= total && len <= total - offset;
}

void
writeChunkEntry(DataStream& s, const ChunkEntry& e) {
  s << e.id << e.offset << e.size << e.rawSize;
  s << static_cast<uint16>(e.format);
  s << static_cast<uint16>(e.compression);
}

void
readChunkEntry(DataStream& s, ChunkEntry& e) {
  s >> e.id >> e.offset >> e.size >> e.rawSize;
  uint16 format = 0;
  uint16 compression = 0;
  s >> format >> compression;
  e.format      = static_cast<ChunkFormat>(format);
  e.compression = static_cast<ChunkCompression>(compression);
}

} // namespace

// ---------------------------------------------------------------------------
// AssetFileWriter
// ---------------------------------------------------------------------------

void
AssetFileWriter::addReference(const UUID& dependency) {
  m_references.push_back(dependency);
}

uint32
AssetFileWriter::addChunk(const void* data,
                          size_t size,
                          ChunkFormat format,
                          ChunkCompression compression) {
  m_chunks.emplace_back();
  PendingChunk& chunk = m_chunks.back();
  chunk.format      = format;
  chunk.compression = compression;
  if (nullptr != data && size > 0) {
    const uint8* bytes = static_cast<const uint8*>(data);
    chunk.data.assign(bytes, bytes + size);
  }
  return static_cast<uint32>(m_chunks.size() - 1);
}

bool
AssetFileWriter::writeTo(DataStream& out) const {
  if (!out.isWriteable()) {
    return false;
  }

  const uint32 chunkCount = static_cast<uint32>(m_chunks.size());
  const uint32 refCount   = static_cast<uint32>(m_references.size());

  // Offsets are fully determined by the fixed region sizes, so we lay them out
  // up front and write in one forward pass (no seek-back to patch the header).
  const uint64 metadataOffset   = kHeaderBytes;
  const uint64 referencesOffset = metadataOffset + kAssetMetadataBytes;
  const uint64 refsBytes        = sizeof(uint32) /*count*/ +
                                  static_cast<uint64>(refCount) * kUuidBytes;
  const uint64 directoryOffset  = referencesOffset + refsBytes;
  const uint64 dirBytes         = static_cast<uint64>(chunkCount) * kChunkEntryBytes;
  uint64 chunkCursor            = directoryOffset + dirBytes;

  // -- Header --
  out.write(kAssetMagic, sizeof(kAssetMagic));
  out << kAssetFormatVersion;
  out << metadataOffset << referencesOffset << directoryOffset << chunkCount;
  // Guards: catch drift between the layout-size constants and what we actually
  // write (e.g. if a metadata field is added but kAssetMetadataBytes isn't).
  SFMX_ASSERT(out.tell() == metadataOffset);

  // -- Metadata + references (the cheap-scan region) --
  writeMetadata(out, m_metadata);
  SFMX_ASSERT(out.tell() == referencesOffset);
  out << refCount;
  for (const UUID& id : m_references) {
    out << id;
  }
  SFMX_ASSERT(out.tell() == directoryOffset);

  // -- Directory --
  for (uint32 i = 0; i < chunkCount; ++i) {
    const PendingChunk& pending = m_chunks[i];
    ChunkEntry entry;
    entry.id          = i;
    entry.offset      = chunkCursor;
    entry.size        = static_cast<uint64>(pending.data.size());
    entry.rawSize     = entry.size;  // v1: chunks are stored uncompressed
    entry.format      = pending.format;
    entry.compression = pending.compression;
    writeChunkEntry(out, entry);
    chunkCursor += entry.size;
  }

  // -- Chunk payloads --
  for (const PendingChunk& pending : m_chunks) {
    if (!pending.data.empty()) {
      out.write(pending.data.data(), pending.data.size());
    }
  }

  return true;
}

// ---------------------------------------------------------------------------
// AssetFileReader
// ---------------------------------------------------------------------------

bool
AssetFileReader::open(const SPtr<DataStream>& stream) {
  m_stream.reset();
  m_references.clear();
  m_chunks.clear();

  if (nullptr == stream || !stream->isReadable()) {
    return false;
  }

  stream->seek(0);
  ansichar magic[4] = {};
  if (stream->read(magic, sizeof(magic)) != sizeof(magic)) {
    return false;
  }
  if (0 != std::memcmp(magic, kAssetMagic, sizeof(magic))) {
    return false;
  }

  uint32 formatVersion = 0;
  *stream >> formatVersion;
  if (formatVersion != kAssetFormatVersion) {
    return false;  // v1: exact match; later, migrate older versions here
  }

  uint64 metadataOffset   = 0;
  uint64 referencesOffset = 0;
  uint64 directoryOffset  = 0;
  uint32 chunkCount       = 0;
  *stream >> metadataOffset >> referencesOffset >> directoryOffset >> chunkCount;

  // Validate every region against the real file size before allocating or
  // seeking, so a corrupt/truncated file fails cleanly instead of doing a huge
  // allocation or reading out of bounds.
  const uint64 fileSize = static_cast<uint64>(stream->size());
  if (!spanFits(metadataOffset, kAssetMetadataBytes, fileSize) ||
      !spanFits(referencesOffset, sizeof(uint32), fileSize) ||
      !spanFits(directoryOffset,
                static_cast<uint64>(chunkCount) * kChunkEntryBytes, fileSize)) {
    return false;
  }

  stream->seek(static_cast<size_t>(metadataOffset));
  readMetadata(*stream, m_metadata);

  stream->seek(static_cast<size_t>(referencesOffset));
  uint32 refCount = 0;
  *stream >> refCount;
  if (!spanFits(referencesOffset,
                sizeof(uint32) + static_cast<uint64>(refCount) * kUuidBytes,
                fileSize)) {
    return false;
  }
  m_references.reserve(refCount);
  for (uint32 i = 0; i < refCount; ++i) {
    UUID id;
    *stream >> id;
    m_references.push_back(id);
  }

  stream->seek(static_cast<size_t>(directoryOffset));
  m_chunks.resize(chunkCount);
  for (uint32 i = 0; i < chunkCount; ++i) {
    readChunkEntry(*stream, m_chunks[i]);
    // Each chunk's payload must lie within the file (keeps readChunk in bounds).
    if (!spanFits(m_chunks[i].offset, m_chunks[i].size, fileSize)) {
      m_chunks.clear();
      m_references.clear();
      return false;
    }
  }

  m_stream = stream;
  return true;
}

bool
AssetFileReader::readChunk(size_t index, Vector<uint8>& out) const {
  if (nullptr == m_stream || index >= m_chunks.size()) {
    return false;
  }

  const ChunkEntry& entry = m_chunks[index];
  out.resize(static_cast<size_t>(entry.size));
  m_stream->seek(static_cast<size_t>(entry.offset));
  const size_t got = out.empty() ? 0 : m_stream->read(out.data(), out.size());
  out.resize(got);
  return got == entry.size;
}

} // namespace sfmx
