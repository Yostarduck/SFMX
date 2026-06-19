#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/Asset.h"

namespace sfmx
{

class AssetFileReader;

/**
 * @brief Turns the chunks of one @c assetType into a usable @ref IAsset.
 *
 * Decode-only in v1: the runtime reads `.sfmxasset` files; importing foreign
 * formats (PNG via stb, FBX via assimp, ...) into a `.sfmxasset` is the cooker's
 * job (TODO) and lives outside the shipped game.
 *
 * A codec is intentionally thin: it knows *which* asset type to instantiate and
 * is keyed by @ref assetType in the @ref AssetCodecRegistry. The *how*, reading
 * chunks, uploading to the GPU , lives in the concrete asset, mirroring how a
 * component owns its own behaviour. Registering a codec (e.g. from a mod DLL) is
 * what teaches the engine a new asset type, with no core recompile.
 */
class SFMX_UTILITY_EXPORT IAssetCodec
{
 public:
  virtual ~IAssetCodec() = default;

  /** @brief The @c assetType (TypeTraits id) this codec decodes. */
  NODISCARD virtual const UUID&
  assetType() const = 0;

  /**
   * @brief Decode @p reader's payload into a new asset.
   * @return The decoded asset (state @ref AssetState::kLoaded on success,
   *         @ref AssetState::kFailed if the bytes were unusable), or @c nullptr
   *         if no asset could be constructed at all.
   */
  NODISCARD virtual SPtr<IAsset>
  decode(AssetFileReader& reader) const = 0;
};

} // namespace sfmx
