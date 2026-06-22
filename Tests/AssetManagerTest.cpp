#include <doctest/doctest.h>

#include <cstdio>
#include <memory>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"
#include "core/FileSystem.h"
#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include "assets/TextureAsset.h"
#include "assets/TextureCodec.h"

using namespace sfmx;

// Note: sfmx::UUID is qualified throughout because on Windows <rpcdce.h> (via
// stduuid's system generator) defines a global ::UUID.

// GL-free dummy asset/codec to exercise the manager's catalog/cache logic.
// Distinct name from AssetCodecTest's DummyAsset to avoid a TypeTraits ODR clash.
struct MgrDummyAsset : AssetT<MgrDummyAsset> {
  void
  loadDummy(const AssetMetadata& meta) {
    setMetadata(meta);
    setState(AssetState::kLoaded);
  }
};

struct MgrDummyCodec : IAssetCodec {
  const sfmx::UUID&
  assetType() const override { return TypeTraits<MgrDummyAsset>::getTypeId(); }

  SPtr<IAsset>
  decode(AssetFileReader& reader) const override {
    SPtr<MgrDummyAsset> asset = MakeShared<MgrDummyAsset>();
    asset->loadDummy(reader.metadata());
    return asset;
  }
};

namespace {

// RAII: guarantees a clean startUp/shutDown even if a REQUIRE throws mid-test.
struct ManagerScope {
  ManagerScope() {
    if (AssetManager::isStarted()) {
      AssetManager::shutDown();
    }
    AssetManager::startUp();
  }
  ~ManagerScope() {
    if (AssetManager::isStarted()) {
      AssetManager::shutDown();
    }
  }
};

FileSystemPath
makeCleanDir(const String& name) {
  const FileSystemPath dir = FileSystem::tempDirectory() / ("sfmx_assetmgr_" + name);
  FileSystem::removeAll(dir);
  return dir;
}

// Write a `.sfmxasset` with an explicit id/type and optional dependency refs.
void
writeAsset(const FileSystemPath& dir,
           const String& name,
           const sfmx::UUID& id,
           const sfmx::UUID& assetType,
           const Vector<sfmx::UUID>& refs = {}) {
  AssetFileWriter writer;
  AssetMetadata meta;
  meta.uuid      = id;
  meta.assetType = assetType;
  std::snprintf(meta.name, sizeof(meta.name), "%s", name.c_str());
  writer.setMetadata(meta);
  for (const sfmx::UUID& ref : refs) {
    writer.addReference(ref);
  }
  const Vector<uint8> payload = {1, 2, 3};
  writer.addChunk(payload.data(), payload.size());

  SPtr<DataStream> out = FileSystem::createAndOpenFile(dir / (name + ".sfmxasset"));
  REQUIRE(out != nullptr);
  REQUIRE(writer.writeTo(*out));
  out->close();
}

} // namespace

TEST_CASE("AssetManager mounts, loads, and caches") {
  const FileSystemPath dir = makeCleanDir("cache");
  const sfmx::UUID dummyType = TypeTraits<MgrDummyAsset>::getTypeId();
  const sfmx::UUID a = sfmx::UUID::createRandom();
  const sfmx::UUID b = sfmx::UUID::createRandom();
  writeAsset(dir, "a", a, dummyType);
  writeAsset(dir, "b", b, dummyType);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<MgrDummyCodec>());

  CHECK(mgr.mount(dir) == 2u);  // Only 2 files there
  CHECK(mgr.isCataloged(a));
  CHECK(String(mgr.metadataOf(a)->name) == "a");

  // Not loaded until requested.
  CHECK_FALSE(mgr.isLoaded(a));
  CHECK(mgr.get(a) == nullptr);

  SPtr<IAsset> first = mgr.load(a);
  REQUIRE(first != nullptr);
  CHECK(first->isLoaded());
  CHECK(mgr.isLoaded(a));

  // Second load hits the cache: same instance, not a re-decode.
  SPtr<IAsset> second = mgr.load(a);
  CHECK(second.get() == first.get());
  CHECK(mgr.get(a).get() == first.get());

  // Eviction drops the cache entry but keeps the catalog.
  mgr.unload(a);
  CHECK_FALSE(mgr.isLoaded(a));
  CHECK(mgr.get(a) == nullptr);
  CHECK(mgr.isCataloged(a));

  FileSystem::removeAll(dir);
}

TEST_CASE("AssetManager returns null for an unknown id") {
  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<MgrDummyCodec>());

  CHECK(mgr.load(sfmx::UUID::createRandom()) == nullptr);
}

TEST_CASE("AssetManager returns null when no codec handles the type") {
  const FileSystemPath dir = makeCleanDir("nocodec");
  const sfmx::UUID id = sfmx::UUID::createRandom();
  // assetType nobody registered a codec for.
  writeAsset(dir, "orphan", id, sfmx::UUID::createFromName("UnhandledType"));

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<MgrDummyCodec>());  // only dummy registered

  REQUIRE(mgr.mount(dir) == 1u);
  CHECK(mgr.load(id) == nullptr);

  FileSystem::removeAll(dir);
}

TEST_CASE("AssetManager resolves references and survives cycles") {
  const FileSystemPath dir = makeCleanDir("refs");
  const sfmx::UUID dummyType = TypeTraits<MgrDummyAsset>::getTypeId();

  SUBCASE("dependency is loaded alongside its parent") {
    const sfmx::UUID parent = sfmx::UUID::createRandom();
    const sfmx::UUID dep    = sfmx::UUID::createRandom();
    writeAsset(dir, "dep", dep, dummyType);
    writeAsset(dir, "parent", parent, dummyType, {dep});

    ManagerScope scope;
    AssetManager& mgr = AssetManager::instance();
    mgr.registerCodec(MakeShared<MgrDummyCodec>());
    REQUIRE(mgr.mount(dir) == 2u);

    REQUIRE(mgr.load(parent) != nullptr);
    CHECK(mgr.isLoaded(dep));  // pulled in while loading parent
  }

  SUBCASE("mutual references do not loop forever") {
    const sfmx::UUID x = sfmx::UUID::createRandom();
    const sfmx::UUID y = sfmx::UUID::createRandom();
    writeAsset(dir, "x", x, dummyType, {y});
    writeAsset(dir, "y", y, dummyType, {x});

    ManagerScope scope;
    AssetManager& mgr = AssetManager::instance();
    mgr.registerCodec(MakeShared<MgrDummyCodec>());
    REQUIRE(mgr.mount(dir) == 2u);

    REQUIRE(mgr.load(x) != nullptr);  // would hang without the cycle guard
    CHECK(mgr.isLoaded(x));
    CHECK(mgr.isLoaded(y));
  }

  FileSystem::removeAll(dir);
}

TEST_CASE("AssetManager loads a real texture through the codec") {
  const FileSystemPath dir = makeCleanDir("texture");

  sf::Image image(sf::Vector2u{4u, 4u}, sf::Color::Red);
  Optional<Vector<uint8>> png = image.saveToMemory("png");
  REQUIRE(png.has_value());

  const sfmx::UUID id = sfmx::UUID::createRandom();
  {
    AssetFileWriter writer;
    AssetMetadata meta;
    meta.uuid      = id;
    meta.assetType = TypeTraits<TextureAsset>::getTypeId();
    std::snprintf(meta.name, sizeof(meta.name), "%s", "hero");
    writer.setMetadata(meta);
    writer.addChunk(png->data(), png->size(), ChunkFormat::kPng);

    SPtr<DataStream> out = FileSystem::createAndOpenFile(dir / "hero.sfmxasset");
    REQUIRE(out != nullptr);
    REQUIRE(writer.writeTo(*out));
    out->close();
  }

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<TextureCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  SPtr<TextureAsset> texture = mgr.load<TextureAsset>(id);
  REQUIRE(texture != nullptr);
  // Uploading to the GPU needs SFML's shared context; valid on a real device.
  CHECK(texture->isLoaded());
  CHECK(texture->texture().getSize() == sf::Vector2u{4u, 4u});

  // Wrong-type query returns null, not a bad cast.
  CHECK(mgr.load<MgrDummyAsset>(id) == nullptr);

  FileSystem::removeAll(dir);
}
