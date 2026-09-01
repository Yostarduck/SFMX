#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/IAssetCodec.h"
#include "assets/LocalizationAsset.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief Codec that decodes a raw chunk into a @ref LocalizationAsset
 *
 * Keyed by @c TypeTraits<LocalizationAsset>::getTypeId(). Stateless — register one
 * instance with the @ref AssetCodecRegistry.
 */
class SFMX_UTILITY_EXPORT LocalizationCodec : public IAssetCodec
{
 public:
  NODISCARD const UUID&
  assetType() const override { return TypeTraits<LocalizationAsset>::getTypeId(); }

  NODISCARD SPtr<IAsset>
  create() const override { return MakeShared<LocalizationAsset>(); }

  NODISCARD SPtr<IAsset>
  decode(AssetFileReader& reader) const override;
};

} // namespace sfmx