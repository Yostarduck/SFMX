#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/IAssetCodec.h"
#include "assets/SoundAsset.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief Codec that decodes audio chunks into a @ref SoundAsset.
 *
 * Keyed by @c TypeTraits<SoundAsset>::getTypeId(). Stateless — register one
 * instance with the @ref AssetCodecRegistry.
 */
class SFMX_UTILITY_EXPORT SoundCodec : public IAssetCodec
{
 public:
  NODISCARD const UUID&
  assetType() const override { return TypeTraits<SoundAsset>::getTypeId(); }

  NODISCARD SPtr<IAsset>
  decode(AssetFileReader& reader) const override;
};

} // namespace sfmx
