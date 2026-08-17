#include "assets/LocalizationCodec.h"
#include "assets/AssetFile.h"

namespace sfmx
{

SPtr<IAsset>
LocalizationCodec::decode(AssetFileReader& reader) const {
  SPtr<LocalizationAsset> asset = MakeShared<LocalizationAsset>();
  // decodeFrom records its own kLoaded/kFailed state; hand the asset back either
  // way so callers can inspect the failure rather than getting nullptr.
  asset->decodeFrom(reader);
  return asset;
}

} // namespace sfmx