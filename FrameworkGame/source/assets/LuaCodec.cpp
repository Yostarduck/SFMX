#include "assets/LuaCodec.h"

#include "assets/AssetFile.h"

namespace sfmx
{

SPtr<IAsset>
LuaCodec::decode(AssetFileReader& reader) const {
  SPtr<LuaAsset> asset = MakeShared<LuaAsset>();
  // decodeFrom records its own kLoaded/kFailed state; hand the asset back either
  // way so callers can inspect the failure rather than getting nullptr.
  asset->decodeFrom(reader);
  return asset;
}

} // namespace sfmx
