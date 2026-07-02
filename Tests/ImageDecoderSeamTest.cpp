#include <doctest/doctest.h>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>

#include "assets/AssetManager.h"
#include "assets/AssetMetadata.h"   // ChunkFormat
#include "assets/IImageDecoder.h"

using namespace sfmx;

// The image-decode seam: format modules register an IImageDecoder keyed by the
// chunk's ChunkFormat tag (written at cook time), and TextureAsset::decodeFrom
// dispatches on that tag — no byte-sniffing, and the decoder never declares its
// own format. This exercises the registry wiring without any format library.

namespace {

// A stand-in for a module decoder (e.g. WebPImageDecoder), with no real codec.
struct FakeImageDecoder : IImageDecoder {
  bool
  decode(const uint8* /*bytes*/, size_t /*size*/, sf::Image& out) const override {
    out = sf::Image(sf::Vector2u{1u, 1u}, sf::Color::Red);  // CPU-side, no GL needed
    return true;
  }
};

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

TEST_CASE("Image decoder seam: registered and found by ChunkFormat tag") {
  AssetManagerScope scope;
  AssetManager& mgr = AssetManager::instance();

  // Nothing registered yet -> the runtime would fall back to SFML loadFromMemory.
  CHECK(mgr.findDecoder<sf::Image>(ChunkFormat::kWebP) == nullptr);

  mgr.registerDecoder<sf::Image>(ChunkFormat::kWebP, MakeShared<FakeImageDecoder>());

  const IImageDecoder* decoder = mgr.findDecoder<sf::Image>(ChunkFormat::kWebP);
  REQUIRE(decoder != nullptr);

  // The tag is the key: an unregistered format still has no decoder (SFML handles
  // PNG/JPG/BMP itself, so the core deliberately registers none for them).
  CHECK(mgr.findDecoder<sf::Image>(ChunkFormat::kPng) == nullptr);

  sf::Image out;
  CHECK(decoder->decode(nullptr, 0, out));
  CHECK(out.getSize() == sf::Vector2u{1u, 1u});
}

TEST_CASE("Image decoder seam: last registration for a format wins") {
  AssetManagerScope scope;
  AssetManager& mgr = AssetManager::instance();

  auto first  = MakeShared<FakeImageDecoder>();
  auto second = MakeShared<FakeImageDecoder>();
  mgr.registerDecoder<sf::Image>(ChunkFormat::kWebP, first);
  mgr.registerDecoder<sf::Image>(ChunkFormat::kWebP, second);

  CHECK(mgr.findDecoder<sf::Image>(ChunkFormat::kWebP) == second.get());
}
