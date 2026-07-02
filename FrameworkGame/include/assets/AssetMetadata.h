#pragma once

#include "core/platform/Prerequisites.h"
#include "utils/UUID.h"

namespace sfmx
{

/**
 * @brief Identity of a chunk's payload encoding — an OPEN, name-derived id.
 *
 * A @ref UUID minted from a short format name (exactly as @ref TypeTraits mints type
 * ids), NOT a closed enum. That openness is the point: any module/DLL can mint its
 * own id for a new encoding via @ref chunkFormatId (e.g. "avif", "opus", "3ds")
 * WITHOUT editing the core — a compile-time enum could never be extended by a
 * runtime-loaded DLL. The well-known engine formats live in namespace
 * @ref ChunkFormat below. @c kRaw is reserved for engine-native/opaque bytes (scene
 * blob, mesh, raw pixels); a foreign media format is never @c kRaw.
 *
 * The NAME is the identity, so a well-known name (see @ref ChunkFormat) must stay
 * stable forever, or old cooked files would resolve to a different id.
 * @see ChunkCompression
 */
using ChunkFormatId = UUID;

/** @brief Mint a @ref ChunkFormatId from a format name (what a module/DLL calls). */
NODISCARD inline ChunkFormatId
chunkFormatId(StringView name) {
  return UUID::createFromName(String(name));
}

/**
 * @brief The well-known engine chunk formats (built-ins). Modules mint their own via
 *        @ref chunkFormatId; these are just the ones the engine ships decoders for
 *        (or, for @c kRaw, treats as engine-native). Values are name-derived, so they
 *        stay identical across builds and machines.
 */
namespace ChunkFormat
{
  inline const ChunkFormatId kRaw  = chunkFormatId("raw");   // engine-native bytes
  inline const ChunkFormatId kWebP = chunkFormatId("webp");
  inline const ChunkFormatId kPng  = chunkFormatId("png");
  inline const ChunkFormatId kOgg  = chunkFormatId("ogg");
  inline const ChunkFormatId kJpeg = chunkFormatId("jpeg");
  inline const ChunkFormatId kBmp  = chunkFormatId("bmp");
  inline const ChunkFormatId kWav  = chunkFormatId("wav");
  inline const ChunkFormatId kFlac = chunkFormatId("flac");
} // namespace ChunkFormat

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
