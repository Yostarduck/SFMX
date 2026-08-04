#include "assets/AchievementCodec.h"
#include "assets/AssetFile.h"

namespace sfmx
{

SPtr<IAsset>
AchievementCodec::decode(AssetFileReader& reader) const {
  SPtr<AchievementAsset> asset = MakeShared<AchievementAsset>();
  // decodeFrom records its own kLoaded/kFailed state; hand the asset back either
  // way so callers can inspect the failure rather than getting nullptr.
  asset->decodeFrom(reader);
  return asset;
}

}