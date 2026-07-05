#include "ImageWebP.h"

#include <webp/decode.h>

#include <SFML/Graphics/Image.hpp>

#include "assets/AssetImporterRegistry.h"
#include "assets/AssetManager.h"
#include "assets/AssetMetadata.h"   // ChunkFormat
#include "assets/IImageDecoder.h"   // IImageDecoder = IDecoder<sf::Image>
#include "assets/TextureAsset.h"

namespace sfmx
{

namespace {

// Decodes WebP bytes into an sf::Image via libwebp. libwebp lives ONLY here (this
// module), never in the core — TextureAsset reaches it through the IDecoder<sf::Image>
// seam, dispatched by the chunk's ChunkFormat tag (no byte-sniffing).
class WebPImageDecoder : public IImageDecoder
{
 public:
  bool
  decode(const uint8* bytes, size_t size, sf::Image& out) const override {
    int width  = 0;
    int height = 0;
    uint8* rgba = WebPDecodeRGBA(bytes, size, &width, &height);
    if (!rgba) {
      return false;  // not a WebP (or corrupt) — libwebp itself is the check.
    }
    // sf::Image copies the pixels, so libwebp's buffer is freed right after.
    out = sf::Image(sf::Vector2u{static_cast<unsigned>(width),
                                 static_cast<unsigned>(height)},
                    rgba);
    WebPFree(rgba);
    return true;
  }
};

} // namespace

namespace imagewebp
{

void
registerModule() {
  // DECODE seam (runtime): the generic registerDecoder<sf::Image>, keyed by the kWebP
  // tag the cooker wrote into the chunk. An audio module would use <sf::SoundBuffer>.
  if (AssetManager::isStarted()) {
    AssetManager::instance().registerDecoder<sf::Image>(
        ChunkFormat::kWebP, MakeShared<WebPImageDecoder>());
  }
  // IMPORT seam (cook): `.webp` sources wrap into a kWebP-tagged TextureAsset chunk.
  if (AssetImporterRegistry::isStarted()) {
    AssetImporterRegistry::instance().registerImporter<TextureAsset>(
        ChunkFormat::kWebP, ".webp");
  }
}

} // namespace imagewebp

} // namespace sfmx
