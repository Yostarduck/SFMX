#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/IAssetCodec.h"
#include "assets/AchievementAsset.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

class SFMX_UTILITY_EXPORT AchievementCodec : public IAssetCodec
{
  public:
  NODISCARD const UUID&
  assetType() const override { return TypeTraits<AchievementAsset>::getTypeId(); }

  NODISCARD SPtr<IAsset>
  create() const override { return MakeShared<AchievementAsset>(); }

  NODISCARD SPtr<IAsset>
  decode(AssetFileReader& reader) const override;
};


} // namespace sfmx