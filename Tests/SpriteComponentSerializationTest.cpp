#include <doctest/doctest.h>

#include <cstdio>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Rect.hpp>

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"
#include "core/MemoryDataStream.h"
#include "core/FileSystem.h"
#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include "assets/TextureAsset.h"
#include "assets/TextureCodec.h"
#include "scene/ComponentRegistry.h"
#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "scene/SceneSerializer.h"
#include "scene/SpriteComponent.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// sfmx::UUID is qualified because on Windows <rpcdce.h> (via stduuid) defines a
// global ::UUID. M5.2: SpriteComponent serializes its texture by asset UUID and
// re-resolves it through the AssetManager — the asset-handle convention.

namespace {

// Idempotent, never-shutdown setup (suite convention — other suites share the
// global MemoryPoolHandler, so we never tear it down here).
void
ensureEnv() {
  if (!MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::startUp(4096);
  }
  MemoryPoolHandler& pools = MemoryPoolHandler::instance();
  if (!pools.hasPool<SceneNode>()) {
    pools.registerPool<SceneNode>(256);
  }
  if (!pools.hasPool<SpriteComponent>()) {
    pools.registerPool<SpriteComponent>(64);
  }

  if (!ComponentRegistry::isStarted()) {
    ComponentRegistry::startUp();
  }
  ComponentRegistry::instance().registerComponent<SpriteComponent>();
}

// RAII: clean startUp/shutDown of the AssetManager even if a REQUIRE throws.
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

// Write an 8x8 blue PNG as a `.sfmxasset` TextureAsset with the given id.
void
writeTextureAsset(const FileSystemPath& dir, const sfmx::UUID& id) {
  sf::Image image(sf::Vector2u{8u, 8u}, sf::Color::Blue);
  Optional<Vector<uint8>> png = image.saveToMemory("png");
  REQUIRE(png.has_value());

  AssetFileWriter writer;
  AssetMetadata meta;
  meta.uuid      = id;
  meta.assetType = TypeTraits<TextureAsset>::getTypeId();
  std::snprintf(meta.name, sizeof(meta.name), "%s", "sprite_tex");
  writer.setMetadata(meta);
  writer.addChunk(png->data(), png->size(), ChunkFormat::kPng);

  SPtr<DataStream> out = FileSystem::createAndOpenFile(dir / "sprite_tex.sfmxasset");
  REQUIRE(out != nullptr);
  REQUIRE(writer.writeTo(*out));
  out->close();
}

} // namespace

TEST_CASE("SpriteComponent round-trips the texture asset UUID and flips (no GL)") {
  ensureEnv();

  Scene scene("s");
  SceneNode* src = scene.createNode("src");
  SceneNode* dst = scene.createNode("dst");
  REQUIRE(src != nullptr);
  REQUIRE(dst != nullptr);

  SpriteComponent* a = src->addComponent<SpriteComponent>();
  REQUIRE(a != nullptr);

  // A random, un-cataloged id never resolves → no sprite is created, so this
  // stays GL-free regardless of whether the AssetManager happens to be running.
  const sfmx::UUID id = sfmx::UUID::createRandom();
  a->setTextureAssetId(id);
  a->flipX(true);

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  SpriteComponent* b = dst->addComponent<SpriteComponent>();
  REQUIRE(b != nullptr);
  b->onDeserialize(blob);

  CHECK(b->getTextureAssetId().toString() == id.toString());
  CHECK(b->getTextureAsset() == nullptr);  // unresolved → no kept-alive asset
  CHECK(b->isFlippedX());
  CHECK_FALSE(b->isFlippedY());
}

TEST_CASE("SpriteComponent re-resolves a real texture by UUID and round-trips rect/color") {
  ensureEnv();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_sprite_test";
  FileSystem::removeAll(dir);

  const sfmx::UUID id = sfmx::UUID::createRandom();
  writeTextureAsset(dir, id);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<TextureCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  Scene scene("s");
  SceneNode* src = scene.createNode("src");
  REQUIRE(src != nullptr);

  SpriteComponent* a = src->addComponent<SpriteComponent>();
  REQUIRE(a != nullptr);
  a->setTextureAsset(mgr.load<TextureAsset>(id));
  REQUIRE(a->getTextureAsset() != nullptr);
  a->setRect(sf::IntRect({1, 2}, {3, 4}));
  a->setColor(sf::Color(10, 20, 30, 40));
  a->setScale(0.1f);
  a->setPosition({5.f, 6.f});
  a->setRotation(30.f);
  a->setOrigin({7.f, 8.f});
  a->flipX(true);
  a->flipY(true);

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  SceneNode* dst = scene.createNode("dst");
  REQUIRE(dst != nullptr);
  SpriteComponent* b = dst->addComponent<SpriteComponent>();
  REQUIRE(b != nullptr);
  b->onDeserialize(blob);  // resolves the texture via the (running) AssetManager

  CHECK(b->getTextureAssetId().toString() == id.toString());
  REQUIRE(b->getTextureAsset() != nullptr);
  CHECK(b->getTextureAsset()->texture().getSize() == sf::Vector2u{8u, 8u});

  const sf::IntRect rect = b->getRect();
  CHECK(rect.position.x == 1);
  CHECK(rect.position.y == 2);
  CHECK(rect.size.x == 3);
  CHECK(rect.size.y == 4);
  CHECK(b->getColor() == sf::Color(10, 20, 30, 40));
  CHECK(b->getScale() == sf::Vector2f{0.1f, 0.1f});
  CHECK(b->getPosition() == sf::Vector2f{5.f, 6.f});
  CHECK(b->getRotationDegrees() == doctest::Approx(30.f));
  CHECK(b->getOrigin() == sf::Vector2f{7.f, 8.f});
  CHECK(b->isFlippedX());
  CHECK(b->isFlippedY());

  FileSystem::removeAll(dir);
}

TEST_CASE("SpriteComponent texture survives a full SceneSerializer round-trip") {
  ensureEnv();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_sprite_scene_test";
  FileSystem::removeAll(dir);

  const sfmx::UUID id = sfmx::UUID::createRandom();
  writeTextureAsset(dir, id);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<TextureCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  Scene src("src");
  SceneNode* node = src.createNode("billboard");
  REQUIRE(node != nullptr);
  SpriteComponent* sprite = node->addComponent<SpriteComponent>();
  REQUIRE(sprite != nullptr);
  sprite->setTextureAsset(mgr.load<TextureAsset>(id));
  REQUIRE(sprite->getTextureAsset() != nullptr);

  MemoryDataStream blob;
  REQUIRE(SceneSerializer::serialize(src, blob));
  blob.seek(0);

  Scene dst("dst");
  REQUIRE(SceneSerializer::deserialize(dst, blob));

  Vector<SceneNode*> found = dst.findNodesByName("billboard");
  REQUIRE(found.size() == 1u);
  SpriteComponent* sprite2 = found[0]->getComponent<SpriteComponent>();
  REQUIRE(sprite2 != nullptr);
  CHECK(sprite2->getTextureAssetId().toString() == id.toString());
  REQUIRE(sprite2->getTextureAsset() != nullptr);
  CHECK(sprite2->getTextureAsset()->texture().getSize() == sf::Vector2u{8u, 8u});

  FileSystem::removeAll(dir);
}
