#include "assets/SoundCodec.h"

#include "assets/AssetFile.h"

namespace sfmx
{

SPtr<IAsset>
SoundCodec::decode(AssetFileReader& reader) const {
  SPtr<SoundAsset> asset = MakeShared<SoundAsset>();
  // decodeFrom records its own kLoaded/kFailed state; we hand the asset back
  // either way so callers can inspect the failure rather than getting nullptr.
  asset->decodeFrom(reader);
  return asset;
}

} // namespace sfmx
