#pragma once

#include "core/platform/Prerequisites.h"
#include "utils/UUID.h"

namespace sfmx
{

/**
 * @brief Payload encoding of a chunk: the true byte format of the chunk.
 *
 * Describes *what the bytes are*, accurately. A self-describing media decoder may
 * still sniff the bytes (e.g. SFML's @c loadFromMemory) instead of trusting this,
 * but the tag stays honest for tooling, re-cook decisions, and any future decoder
 * that dispatches on it. @c kRaw is reserved for genuinely engine-native/opaque
 * bytes (scene blob, mesh, raw pixels); a foreign media format is never @c kRaw.
 *
 * Serialized to disk as @c uint16, so values are **append-only**: add new formats
 * at the end, never renumber, or old cooked files would misread.
 * @see ChunkCompression
 */
enum class ChunkFormat : uint16 {
  kRaw  = 0,   // engine-native bytes (scene blob, mesh, raw pixels, ...)
  kWebP = 1,
  kPng  = 2,
  kOgg  = 3,
  kJpeg = 4,
  kBmp  = 5,
  kWav  = 6,
  kFlac = 7,
};

/** @brief Whether a chunk's on-disk bytes are compressed (and with what). */
enum class ChunkCompression : uint16 {
  kNone = 0,
  kZstd = 1,
  kLZ4  = 2,
};

constexpr size_t kAssetTypeNameLength   = 32;
constexpr size_t kAssetNameLength       = 64;
constexpr size_t kAssetSourcePathLength = 256;

/** @brief On-disk size of @ref AssetMetadata, serialized field-by-field. */
constexpr size_t kAssetMetadataBytes =
    kUuidBytes +     // uuid       (serialized size, NOT sizeof(UUID))
    kUuidBytes +     // assetType
    sizeof(uint64) + // creationTime
    sizeof(uint32) + // version
    kAssetTypeNameLength +
    kAssetNameLength +
    kAssetSourcePathLength;

/**
 * @brief Fixed-size descriptor at the head of every `.sfmxasset`, cheap to scan
 *        without touching the payload.
 *
 * Serialized field-by-field (never a raw struct blit), so the on-disk layout is
 * stable regardless of compiler padding. The on-disk size is fixed
 * (@ref kAssetMetadataBytes): bounded char arrays + a UUID pair + scalars.
 *
 * The file's own path is intentionally NOT stored here; the AssetManager knows
 * it from the directory scan, and embedding it would desync if the file moves.
 * @c sourcePath is the original authoring file (for future re-cook), empty if none.
 */
struct AssetMetadata {
  UUID     uuid;                                       // stable asset id
  UUID     assetType;                                  // TypeTraits<XAsset> type id
  uint64   creationTime = 0;
  uint32   version      = 1;                           // per-asset schema version
  ansichar typeName[kAssetTypeNameLength]     = "Unknown";
  ansichar name[kAssetNameLength]             = "Unnamed";
  ansichar sourcePath[kAssetSourcePathLength] = "";
};

} // namespace sfmx
