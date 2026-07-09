#include "assets/FontCodec.h"

#include "assets/AssetFile.h"

namespace sfmx
{
SPtr<IAsset>
FontCodec::decode(AssetFileReader& reader) const {
  SPtr<FontAsset> asset = MakeShared<FontAsset>();
  // decodeFrom records its own kLoaded/kFailed state; we hand the asset back
  // either way so callers can inspect the failure rather than getting nullptr.
  asset->decodeFrom(reader);
  return asset;
} 

} // namespace sfmx
