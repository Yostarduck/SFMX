#include "assets/SoundAsset.h"

#include "assets/AssetFile.h"

namespace sfmx
{

bool
SoundAsset::decodeFrom(AssetFileReader& reader) {
  setMetadata(reader.metadata());

  if (reader.chunkCount() == 0) {
    setState(AssetState::kFailed);
    return false;
  }

  // Pull the encoded audio bytes (Ogg/WAV/FLAC) and let SFML decode them to PCM.
  // The scratch buffer is load-time only; this never runs in the game loop.
  Vector<uint8> bytes;
  if (!reader.readChunk(0, bytes) || bytes.empty()) {
    setState(AssetState::kFailed);
    return false;
  }

  const bool ok = m_buffer.loadFromMemory(bytes.data(), bytes.size());
  setState(ok ? AssetState::kLoaded : AssetState::kFailed);
  return ok;
}

} // namespace sfmx
