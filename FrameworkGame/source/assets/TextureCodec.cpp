#include "assets/TextureCodec.h"

#include "assets/AssetFile.h"

namespace sfmx
{

SPtr<IAsset>
TextureCodec::decode(AssetFileReader& reader) const {
  SPtr<TextureAsset> asset = MakeShared<TextureAsset>();
  // decodeFrom records its own kLoaded/kFailed state; we hand the asset back
  // either way so callers can inspect the failure rather than getting nullptr.
  asset->decodeFrom(reader);
  return asset;
}

} // namespace sfmx
