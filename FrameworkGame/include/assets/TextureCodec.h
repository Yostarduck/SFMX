#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/IAssetCodec.h"
#include "assets/TextureAsset.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief Codec that decodes image chunks into a @ref TextureAsset.
 *
 * Keyed by @c TypeTraits<TextureAsset>::getTypeId(). Stateless — register one
 * instance with the @ref AssetCodecRegistry.
 */
class SFMX_UTILITY_EXPORT TextureCodec : public IAssetCodec
{
 public:
  NODISCARD const UUID&
  assetType() const override { return TypeTraits<TextureAsset>::getTypeId(); }

  NODISCARD SPtr<IAsset>
  decode(AssetFileReader& reader) const override;
};

} // namespace sfmx
