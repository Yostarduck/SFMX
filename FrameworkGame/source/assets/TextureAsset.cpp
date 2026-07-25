#include "assets/TextureAsset.h"

#include <SFML/Graphics/Image.hpp>

#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include "assets/IImageDecoder.h"

namespace sfmx
{

bool
TextureAsset::decodeFrom(AssetFileReader& reader) {
  // Synchronous path: the CPU decode and the GPU upload both run here, on the
  // caller's (GL-owning) thread. Split into the same two phases the async path uses.
  return decodeCPU(reader) && finalize();
}

bool
TextureAsset::decodeCPU(AssetFileReader& reader) {
  // WORKER thread: decode the image bytes to a CPU sf::Image only. No GL/GPU here —
  // the upload is deferred to finalize() on the main thread.
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

  const bool ok = decoder
                      ? decoder->decode(bytes.data(), bytes.size(), m_pendingImage)
                      : m_pendingImage.loadFromMemory(bytes.data(), bytes.size());

  // Stay kLoading on success (the GPU upload in finalize completes the load);
  // fail fast otherwise so the pump skips the upload.
  setState(ok ? AssetState::kLoading : AssetState::kFailed);
  return ok;
}

bool
TextureAsset::finalize() {
  // MAIN thread: upload the CPU image to the GPU texture (needs the GL context).
  const bool ok = m_texture.loadFromImage(m_pendingImage);
  m_pendingImage = sf::Image();  // release the transient CPU pixels
  setState(ok ? AssetState::kLoaded : AssetState::kFailed);
  return ok;
}

} // namespace sfmx
