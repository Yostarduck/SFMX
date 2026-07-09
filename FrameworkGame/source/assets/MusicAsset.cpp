#include "assets/MusicAsset.h"

#include "assets/AssetFile.h"

namespace sfmx
{

bool
MusicAsset::decodeFrom(AssetFileReader& reader) {
  setMetadata(reader.metadata());

  if (reader.chunkCount() == 0) {
    setState(AssetState::kFailed);
    return false;
  }

  // Keep the encoded bytes exactly as cooked — NO decode. sf::Music streams from
  // this blob at playback time; it stays resident for the asset's lifetime so the
  // openFromMemory sources never dangle. Load-time only, so the read is fine.
  if (!reader.readChunk(0, m_encodedBytes) || m_encodedBytes.empty()) {
    setState(AssetState::kFailed);
    return false;
  }

  setState(AssetState::kLoaded);
  return true;
}

} // namespace sfmx
