#include <doctest/doctest.h>

#include <SFML/Graphics/Image.hpp>

#include "assets/AssetManager.h"
#include "assets/AssetMetadata.h"   // ChunkFormat
#include "assets/IImageDecoder.h"   // IImageDecoder = IDecoder<sf::Image>

#include "ImageWebP.h"

using namespace sfmx;

// Verifies the SFMX::ImageWebP module: it links libwebp (decode-only), and
// imagewebp::registerModule wires a WebP IDecoder<sf::Image> into the AssetManager
// keyed by ChunkFormat::kWebP. A positive decode of a real .webp is covered by the
// build+run verification (this environment has no WebP encoder/sample to embed, and
// linking libwebp's encoder here would duplicate the decoder symbols the module
// already provides). Here we assert the wiring and that libwebp rejects non-WebP.

namespace {

struct AssetManagerScope {
  AssetManagerScope() {
    if (AssetManager::isStarted()) {
      AssetManager::shutDown();
    }
    AssetManager::startUp();
  }
  ~AssetManagerScope() {
    if (AssetManager::isStarted()) {
      AssetManager::shutDown();
    }
  }
};

} // namespace

TEST_CASE("ImageWebP module registers a decoder for the kWebP tag") {
  AssetManagerScope scope;

  // No AssetImporterRegistry started here, so registerModule's import-rule half is a
  // no-op; only the decoder is registered (the runtime path).
  CHECK(AssetManager::instance().findDecoder<sf::Image>(ChunkFormat::kWebP) == nullptr);

  imagewebp::registerModule();

  const IImageDecoder* decoder =
      AssetManager::instance().findDecoder<sf::Image>(ChunkFormat::kWebP);
  REQUIRE(decoder != nullptr);

  // libwebp itself is the format check: non-WebP bytes decode to false (no sniff).
  const uint8 notWebP[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  sf::Image out;
  CHECK_FALSE(decoder->decode(notWebP, sizeof(notWebP), out));
}
