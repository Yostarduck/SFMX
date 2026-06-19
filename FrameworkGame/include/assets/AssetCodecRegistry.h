#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/IAssetCodec.h"

namespace sfmx
{

class AssetFileReader;

/**
 * @brief Maps @c assetType (UUID) -> @ref IAssetCodec and drives generic decode.
 *
 * Loading an asset is type-agnostic: read the file's @c assetType, look up the
 * codec, hand it the reader. This is the extensibility seam, a mod/DLL calls
 * @ref registerCodec to teach the engine a new asset type without a core
 * recompile; a missing codec is a clean failure, never a crash.
 *
 * Standalone (not a @ref Module): the AssetManager (TODO) will own one of these.
 */
class SFMX_UTILITY_EXPORT AssetCodecRegistry
{
 public:
  /**
   * @brief Register @p codec under its own @ref IAssetCodec::assetType.
   *
   * Takes the @c SPtr by value and sinks it (move into the map): a fresh
   * @c MakeShared codec moves in with no refcount traffic.
   */
  void
  registerCodec(SPtr<IAssetCodec> codec);

  /**
   * @brief The codec for @p assetType, or @c nullptr if none is registered.
   *
   * Non-owning observer (the registry keeps ownership). @c const because codecs
   * are used read-only — their whole API (@ref IAssetCodec::decode, ...) is const.
   */
  NODISCARD const IAssetCodec*
  find(const UUID& assetType) const;

  /** @brief Whether a codec is registered for @p assetType. */
  NODISCARD bool
  hasCodec(const UUID& assetType) const;

  /**
   * @brief Decode the asset @p reader points at, dispatching on its @c assetType.
   * @return The decoded asset, or @c nullptr if no codec handles that type.
   */
  NODISCARD SPtr<IAsset>
  decode(AssetFileReader& reader) const;

 private:
  UnorderedMap<UUID, SPtr<IAssetCodec>> m_codecs;
};

} // namespace sfmx
