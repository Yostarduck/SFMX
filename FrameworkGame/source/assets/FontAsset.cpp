#include "assets/FontAsset.h"

#include <SFML/Graphics/Font.hpp>
#include "assets/AssetFile.h"
#include "assets/AssetManager.h"

namespace sfmx
{

bool 
FontAsset::decodeFrom(AssetFileReader& reader) {
  setMetadata(reader.metadata());

  if (reader.chunkCount() == 0) {
    setState(AssetState::kFailed);
    return false;
  }

  // The scratch buffer is load-time only; this (should) never run in the game loop.
  m_bytes.clear();
  if (!reader.readChunk(0, m_bytes) || m_bytes.empty()) {
    setState(AssetState::kFailed);
    return false;
  }

  bool ok = m_font.openFromMemory(m_bytes.data(), m_bytes.size());
  setState(ok ? AssetState::kLoaded : AssetState::kFailed);
  return ok;
}

} // namespace sfmx