#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/IAssetCodec.h"
#include "assets/MusicAsset.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief Codec that wraps a music chunk into a @ref MusicAsset (no decode).
 *
 * Keyed by @c TypeTraits<MusicAsset>::getTypeId(). Stateless — register one
 * instance with the @ref AssetCodecRegistry. Unlike @ref SoundCodec, it does not
 * turn the bytes into PCM; it hands the encoded blob to the asset for streaming.
 */
class SFMX_UTILITY_EXPORT MusicCodec : public IAssetCodec
{
 public:
  NODISCARD const UUID&
  assetType() const override { return TypeTraits<MusicAsset>::getTypeId(); }

  NODISCARD SPtr<IAsset>
  decode(AssetFileReader& reader) const override;
};

} // namespace sfmx
