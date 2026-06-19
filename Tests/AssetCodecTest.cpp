#include <doctest/doctest.h>

#include <memory>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>

#include "core/platform/Prerequisites.h"
#include "core/MemoryDataStream.h"
#include "assets/AssetFile.h"
#include "assets/AssetCodecRegistry.h"
#include "assets/TextureAsset.h"
#include "assets/TextureCodec.h"

using namespace sfmx;

// Note: sfmx::UUID is qualified because on Windows <rpcdce.h> (via stduuid's
// system generator, pulled in through AssetMetadata.h) defines a global ::UUID.

// A trivial asset/codec pair, GL-free, to exercise registry dispatch on its own.
struct DummyAsset : AssetT<DummyAsset> {
  void
  loadDummy(const AssetMetadata& meta) {
    setMetadata(meta);
    setState(AssetState::kLoaded);
  }
};

struct DummyCodec : IAssetCodec {
  const sfmx::UUID&
  assetType() const override { return TypeTraits<DummyAsset>::getTypeId(); }

  SPtr<IAsset>
  decode(AssetFileReader& reader) const override {
    SPtr<DummyAsset> asset = MakeShared<DummyAsset>();
    asset->loadDummy(reader.metadata());
    return asset;
  }
};

namespace {

// Build an in-memory .sfmxasset with the given assetType and a single chunk.
SPtr<MemoryDataStream>
makeAssetStream(const sfmx::UUID& assetType,
                const Vector<uint8>& chunk,
                ChunkFormat format = ChunkFormat::kRaw) {
  AssetFileWriter writer;
  AssetMetadata meta;
  meta.uuid      = sfmx::UUID::createRandom();
  meta.assetType = assetType;
  writer.setMetadata(meta);
  writer.addChunk(chunk.data(), chunk.size(), format);

  auto buffer = MakeShared<MemoryDataStream>();
  REQUIRE(writer.writeTo(*buffer));
  buffer->seek(0);
  return buffer;
}

} // namespace

TEST_CASE("AssetCodecRegistry dispatches on assetType") {
  AssetCodecRegistry registry;
  registry.registerCodec(MakeShared<DummyCodec>());

  CHECK(registry.hasCodec(TypeTraits<DummyAsset>::getTypeId()));
  CHECK_FALSE(registry.hasCodec(sfmx::UUID::createRandom()));
  CHECK(registry.find(TypeTraits<DummyAsset>::getTypeId()) != nullptr);

  // Known type decodes through its codec.
  auto stream = makeAssetStream(TypeTraits<DummyAsset>::getTypeId(), {1, 2, 3});
  AssetFileReader reader;
  REQUIRE(reader.open(stream));

  SPtr<IAsset> asset = registry.decode(reader);
  REQUIRE(asset != nullptr);
  CHECK(asset->isLoaded());
  CHECK(asset->typeId() == TypeTraits<DummyAsset>::getTypeId());
  CHECK(std::static_pointer_cast<DummyAsset>(asset) != nullptr);
}

TEST_CASE("AssetCodecRegistry returns null for an unregistered assetType") {
  AssetCodecRegistry registry;
  registry.registerCodec(MakeShared<TextureCodec>());

  auto stream = makeAssetStream(sfmx::UUID::createRandom(), {0xAA, 0xBB});
  AssetFileReader reader;
  REQUIRE(reader.open(stream));

  CHECK(registry.decode(reader) == nullptr);
}

TEST_CASE("TextureCodec decodes a PNG chunk into a TextureAsset") {
  // CPU-only PNG generation (no GL context needed to encode).
  sf::Image image(sf::Vector2u{4u, 4u}, sf::Color::Red);
  Optional<Vector<uint8>> png = image.saveToMemory("png");
  REQUIRE(png.has_value());

  Vector<uint8> bytes(png->begin(), png->end());
  auto stream = makeAssetStream(TypeTraits<TextureAsset>::getTypeId(),
                                bytes, ChunkFormat::kPng);

  AssetCodecRegistry registry;
  registry.registerCodec(MakeShared<TextureCodec>());

  AssetFileReader reader;
  REQUIRE(reader.open(stream));

  SPtr<IAsset> asset = registry.decode(reader);
  REQUIRE(asset != nullptr);

  SPtr<TextureAsset> texture = std::static_pointer_cast<TextureAsset>(asset);
  REQUIRE(texture != nullptr);
  // Uploading to the GPU needs SFML's shared context; valid on a real device.
  CHECK(texture->isLoaded());
  CHECK(texture->texture().getSize() == sf::Vector2u{4u, 4u});
}

TEST_CASE("TextureAsset fails cleanly on bad / missing image data") {
  AssetCodecRegistry registry;
  registry.registerCodec(MakeShared<TextureCodec>());

  SUBCASE("garbage bytes -> kFailed, no crash") {
    const Vector<uint8> garbage(32, 0x77u);
    auto stream = makeAssetStream(TypeTraits<TextureAsset>::getTypeId(),
                                  garbage, ChunkFormat::kPng);
    AssetFileReader reader;
    REQUIRE(reader.open(stream));

    SPtr<IAsset> asset = registry.decode(reader);
    REQUIRE(asset != nullptr);
    CHECK_FALSE(asset->isLoaded());
    CHECK(asset->state() == AssetState::kFailed);
  }

  SUBCASE("no chunks -> kFailed") {
    AssetFileWriter writer;
    AssetMetadata meta;
    meta.assetType = TypeTraits<TextureAsset>::getTypeId();
    writer.setMetadata(meta);  // no chunks added

    auto buffer = MakeShared<MemoryDataStream>();
    REQUIRE(writer.writeTo(*buffer));
    buffer->seek(0);

    AssetFileReader reader;
    REQUIRE(reader.open(buffer));

    SPtr<IAsset> asset = registry.decode(reader);
    REQUIRE(asset != nullptr);
    CHECK(asset->state() == AssetState::kFailed);
  }
}
