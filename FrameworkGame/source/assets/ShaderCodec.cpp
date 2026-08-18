#include "assets/ShaderCodec.h"

#include "assets/AssetFile.h"

namespace sfmx
{

SPtr<IAsset>
ShaderCodec::decode(AssetFileReader& reader) const {
  SPtr<ShaderAsset> asset = MakeShared<ShaderAsset>();
  // decodeFrom records its own kLoaded/kFailed state; hand the asset back either
  // way so callers can inspect a compile failure rather than getting nullptr.
  asset->decodeFrom(reader);
  return asset;
}

} // namespace sfmx
