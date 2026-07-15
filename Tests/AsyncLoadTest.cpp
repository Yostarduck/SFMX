#include <doctest/doctest.h>

#include <chrono>
#include <thread>

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

// Note: sfmx::UUID is qualified because on Windows <rpcdce.h> defines a global ::UUID.

namespace {

// RAII startUp/shutDown (also spawns/joins the async worker via on{Start,Shut}Down).
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
  const FileSystemPath dir = FileSystem::tempDirectory() / ("sfmx_async_" + name);
  FileSystem::removeAll(dir);
  return dir;
}

// Write a real PNG-backed TextureAsset `.sfmxasset` and return its id.
sfmx::UUID
writeTextureAsset(const FileSystemPath& dir, const String& name) {
  sf::Image image(sf::Vector2u{4u, 4u}, sf::Color::Red);
  Optional<Vector<uint8>> png = image.saveToMemory("png");
  REQUIRE(png.has_value());

  const sfmx::UUID id = sfmx::UUID::createRandom();
  AssetFileWriter writer;
  AssetMetadata meta;
  meta.uuid      = id;
  meta.assetType = TypeTraits<TextureAsset>::getTypeId();
  std::snprintf(meta.name, sizeof(meta.name), "%s", name.c_str());
  writer.setMetadata(meta);
  writer.addChunk(png->data(), png->size(), ChunkFormat::kPng);

  SPtr<DataStream> out = FileSystem::createAndOpenFile(dir / (name + ".sfmxasset"));
  REQUIRE(out != nullptr);
  REQUIRE(writer.writeTo(*out));
  out->close();
  return id;
}

// Pump finalize() (as a frame loop would) until `asset` finishes or we time out.
bool
pumpUntilLoaded(AssetManager& mgr, const SPtr<IAsset>& asset, int maxFrames = 500) {
  for (int i = 0; i < maxFrames; ++i) {
    mgr.finalize();
    if (asset->state() != AssetState::kLoading) {
      return asset->isLoaded();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return false;
}

} // namespace

TEST_CASE("loadAsync decodes on a worker and finalizes via the pump") {
  const FileSystemPath dir = makeCleanDir("basic");
  const sfmx::UUID id = writeTextureAsset(dir, "hero");

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<TextureCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  int  callbackCount = 0;
  bool sawLoaded     = false;
  SPtr<IAsset> handle = mgr.loadAsync(id, [&](SPtr<IAsset> a) {
    ++callbackCount;
    sawLoaded = (a != nullptr && a->isLoaded());
  });

  // Immediately in flight, and the callback has NOT fired (only the pump fires it).
  REQUIRE(handle != nullptr);
  CHECK(handle->state() == AssetState::kLoading);
  CHECK(callbackCount == 0);
  CHECK_FALSE(mgr.isLoaded(id));

  REQUIRE(pumpUntilLoaded(mgr, handle));
  CHECK(handle->isLoaded());
  CHECK(mgr.isLoaded(id));                 // cached only after finalize
  CHECK(callbackCount == 1);               // fired exactly once
  CHECK(sawLoaded);

  SPtr<TextureAsset> tex = std::static_pointer_cast<TextureAsset>(handle);
  CHECK(tex->texture().getSize() == sf::Vector2u{4u, 4u});

  FileSystem::removeAll(dir);
}

TEST_CASE("loadAsync coalesces duplicate requests into one decode") {
  const FileSystemPath dir = makeCleanDir("dedup");
  const sfmx::UUID id = writeTextureAsset(dir, "hero");

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<TextureCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  int firstCount = 0, secondCount = 0;
  SPtr<IAsset> a = mgr.loadAsync(id, [&](SPtr<IAsset>) { ++firstCount; });
  SPtr<IAsset> b = mgr.loadAsync(id, [&](SPtr<IAsset>) { ++secondCount; });

  // Same in-flight asset handed to both callers (one job, not two).
  CHECK(a.get() == b.get());

  REQUIRE(pumpUntilLoaded(mgr, a));
  CHECK(firstCount == 1);   // both callbacks fire...
  CHECK(secondCount == 1);  // ...for the single decode

  FileSystem::removeAll(dir);
}

TEST_CASE("loadAsync on a cached asset defers the callback to the pump") {
  const FileSystemPath dir = makeCleanDir("cachehit");
  const sfmx::UUID id = writeTextureAsset(dir, "hero");

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<TextureCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  SPtr<IAsset> first = mgr.loadAsync(id);
  REQUIRE(pumpUntilLoaded(mgr, first));
  REQUIRE(mgr.isLoaded(id));

  // Now cached: the callback must still fire from the pump, not synchronously.
  int count = 0;
  SPtr<IAsset> again = mgr.loadAsync(id, [&](SPtr<IAsset>) { ++count; });
  CHECK(again.get() == first.get());
  CHECK(count == 0);        // deferred
  mgr.finalize();
  CHECK(count == 1);        // fired on the next pump

  FileSystem::removeAll(dir);
}

TEST_CASE("loadAsync reports an unknown id as an immediate failure") {
  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<TextureCodec>());

  int count = 0;
  SPtr<IAsset> got;
  SPtr<IAsset> handle = mgr.loadAsync(sfmx::UUID::createRandom(),
    [&](SPtr<IAsset> a) { ++count; got = a; });

  CHECK(handle == nullptr);
  CHECK(count == 1);        // a hard failure reports right away (nothing to decode)
  CHECK(got == nullptr);
}
