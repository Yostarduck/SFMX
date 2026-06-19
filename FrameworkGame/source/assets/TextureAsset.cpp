#include "assets/TextureAsset.h"

#include "assets/AssetFile.h"

namespace sfmx
{

bool
TextureAsset::decodeFrom(AssetFileReader& reader) {
  setMetadata(reader.metadata());

  if (reader.chunkCount() == 0) {
    setState(AssetState::kFailed);
    return false;
  }

  // Pull the encoded image bytes (PNG/JPG) and let SFML decode + upload them.
  // The scratch buffer is load-time only; this (should) never runs in the game loop.
  Vector<uint8> bytes;
  if (!reader.readChunk(0, bytes) || bytes.empty()) {
    setState(AssetState::kFailed);
    return false;
  }

  const bool ok = m_texture.loadFromMemory(bytes.data(), bytes.size());
  setState(ok ? AssetState::kLoaded : AssetState::kFailed);
  return ok;
}

} // namespace sfmx
