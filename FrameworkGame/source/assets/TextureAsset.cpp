#include "assets/TextureAsset.h"

#include <SFML/Graphics/Image.hpp>

#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include "assets/IImageDecoder.h"

namespace sfmx
{

bool
TextureAsset::decodeFrom(AssetFileReader& reader) {
  setMetadata(reader.metadata());

  if (reader.chunkCount() == 0) {
    setState(AssetState::kFailed);
    return false;
  }

  // The scratch buffer is load-time only; this (should) never runs in the game loop.
  Vector<uint8> bytes;
  if (!reader.readChunk(0, bytes) || bytes.empty()) {
    setState(AssetState::kFailed);
    return false;
  }

  // Formats a registered module handles (e.g. WebP) go through the image-decoder
  // seam so the core never links their library; everything SFML sniffs natively
  // (PNG/JPG/BMP) falls through to loadFromMemory. A kWebP chunk with no module
  // registered simply fails to decode → kFailed, cleanly.
  const IImageDecoder* decoder =
      AssetManager::isStarted()
          ? AssetManager::instance().findDecoder<sf::Image>(reader.chunk(0).format)
          : nullptr;

  bool ok = false;
  if (decoder) {
    sf::Image image;
    ok = decoder->decode(bytes.data(), bytes.size(), image) &&
         m_texture.loadFromImage(image);
  }
  else {
    ok = m_texture.loadFromMemory(bytes.data(), bytes.size());
  }

  setState(ok ? AssetState::kLoaded : AssetState::kFailed);
  return ok;
}

} // namespace sfmx
