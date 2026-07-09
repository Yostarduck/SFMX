#include "assets/MusicCodec.h"

#include "assets/AssetFile.h"

namespace sfmx
{

SPtr<IAsset>
MusicCodec::decode(AssetFileReader& reader) const {
  SPtr<MusicAsset> asset = MakeShared<MusicAsset>();
  // decodeFrom records its own kLoaded/kFailed state; we hand the asset back
  // either way so callers can inspect the failure rather than getting nullptr.
  asset->decodeFrom(reader);
  return asset;
}

} // namespace sfmx
