#pragma once
#include "core/platform/Prerequisites.h"
#include "assets/IAssetCodec.h"
#include "assets/FontAsset.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

class SFMX_UTILITY_EXPORT FontCodec : public IAssetCodec
{
 public:
  NODISCARD const UUID&
  assetType() const override { return TypeTraits<FontAsset>::getTypeId(); }

  NODISCARD SPtr<IAsset>
  decode(AssetFileReader& reader) const override;
};

} // namespace sfmx

