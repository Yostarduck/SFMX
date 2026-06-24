#include <doctest/doctest.h>

#include <cstdio>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Angle.hpp>

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"
#include "core/MemoryDataStream.h"
#include "core/FileSystem.h"
#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include "assets/TextureAsset.h"
#include "assets/TextureCodec.h"
#include "scene/ComponentRegistry.h"
#include "scene/ParticleSystemComponent.h"
#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "scene/SceneSerializer.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// sfmx::UUID is qualified because on Windows <rpcdce.h> (via stduuid) defines a
// global ::UUID. M5.3: ParticleSystemComponent serializes its EmitterConfig, sort
// mode and world-space flag; the texture is stored by asset UUID and re-resolved
// through the AssetManager. Live particles/timers are transient and not saved.

namespace {

// Idempotent, never-shutdown setup (suite convention — other suites share the
// global MemoryPoolHandler). setConfig() asserts the shared Particle pool is at
// least maxParticles, so size it generously here.
void
ensureEnv() {
  if (!MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::startUp(4096);
  }
  MemoryPoolHandler& pools = MemoryPoolHandler::instance();
  if (!pools.hasPool<SceneNode>()) {
    pools.registerPool<SceneNode>(256);
  }
  if (!pools.hasPool<ParticleSystemComponent>()) {
    pools.registerPool<ParticleSystemComponent>(32);
  }
  if (!pools.hasPool<Particle>()) {
    pools.registerPool<Particle>(512);
  }

  if (!ComponentRegistry::isStarted()) {
    ComponentRegistry::startUp();
  }
  ComponentRegistry::instance().registerComponent<ParticleSystemComponent>();
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
  std::snprintf(meta.name, sizeof(meta.name), "%s", "particle_tex");
  writer.setMetadata(meta);
  writer.addChunk(png->data(), png->size(), ChunkFormat::kPng);

  SPtr<DataStream> out = FileSystem::createAndOpenFile(dir / "particle_tex.sfmxasset");
  REQUIRE(out != nullptr);
  REQUIRE(writer.writeTo(*out));
  out->close();
}

// A config with distinctive, non-default values across every field group.
EmitterConfig
makeConfig() {
  EmitterConfig cfg;
  cfg.maxParticles            = 64;
  cfg.positionOffset          = {1.f, 2.f};
  cfg.gravity                 = {-40.f, 9.8f};
  cfg.startSize               = {20.f, 18.f};
  cfg.endSize                 = {2.f, 1.f};
  cfg.blendMode               = sf::BlendAdd;
  cfg.emissionRate            = 40.f;
  cfg.positionVariance        = 3.5f;
  cfg.direction               = sf::degrees(90.f);
  cfg.directionVariance       = sf::degrees(15.f);
  cfg.speed                   = 120.f;
  cfg.speedVariance           = 40.f;
  cfg.startRotation           = sf::degrees(30.f);
  cfg.startRotationVariance   = sf::degrees(360.f);
  cfg.angularVelocity         = 90.f;
  cfg.angularVelocityVariance = 45.f;
  cfg.startColor              = sf::Color(255, 200, 80, 255);
  cfg.endColor                = sf::Color(255, 50, 0, 0);
  cfg.lifetime                = 5.0f;
  cfg.lifetimeVariance        = 0.5f;
  cfg.duration                = 3.0f;
  cfg.loop                    = true;
  return cfg;
}

} // namespace

TEST_CASE("ParticleSystemComponent round-trips its config, blend, sort and world-space (no GL)") {
  ensureEnv();

  Scene scene("s");
  SceneNode* src = scene.createNode("src");
  REQUIRE(src != nullptr);

  ParticleSystemComponent* a = src->addComponent<ParticleSystemComponent>();
  REQUIRE(a != nullptr);
  a->setConfig(makeConfig());            // null textureAssetId → stays GL-free
  a->setSortMode(ParticleSortMode::kBackToFront);
  a->setWorldSpace(true);

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  SceneNode* dst = scene.createNode("dst");
  REQUIRE(dst != nullptr);
  ParticleSystemComponent* b = dst->addComponent<ParticleSystemComponent>();
  REQUIRE(b != nullptr);
  b->onDeserialize(blob);

  const EmitterConfig& c = b->getConfig();
  CHECK(c.maxParticles == 64);
  CHECK(c.positionOffset.x == doctest::Approx(1.f));
  CHECK(c.positionOffset.y == doctest::Approx(2.f));
  CHECK(c.gravity.x == doctest::Approx(-40.f));
  CHECK(c.gravity.y == doctest::Approx(9.8f));
  CHECK(c.startSize.x == doctest::Approx(20.f));
  CHECK(c.endSize.y == doctest::Approx(1.f));
  CHECK(c.blendMode == sf::BlendAdd);
  CHECK(c.emissionRate == doctest::Approx(40.f));
  CHECK(c.positionVariance == doctest::Approx(3.5f));
  CHECK(c.direction.asDegrees() == doctest::Approx(90.f));
  CHECK(c.directionVariance.asDegrees() == doctest::Approx(15.f));
  CHECK(c.speed == doctest::Approx(120.f));
  CHECK(c.speedVariance == doctest::Approx(40.f));
  CHECK(c.startRotation.asDegrees() == doctest::Approx(30.f));
  CHECK(c.startRotationVariance.asDegrees() == doctest::Approx(360.f));
  CHECK(c.angularVelocity == doctest::Approx(90.f));
  CHECK(c.angularVelocityVariance == doctest::Approx(45.f));
  CHECK(c.startColor == sf::Color(255, 200, 80, 255));
  CHECK(c.endColor == sf::Color(255, 50, 0, 0));
  CHECK(c.lifetime == doctest::Approx(5.0f));
  CHECK(c.lifetimeVariance == doctest::Approx(0.5f));
  CHECK(c.duration == doctest::Approx(3.0f));
  CHECK(c.loop == true);
  CHECK(c.texture == nullptr);

  CHECK(b->getSortMode() == ParticleSortMode::kBackToFront);
  CHECK(b->isWorldSpace() == true);
  CHECK(b->getMaxParticles() == 64);
}

TEST_CASE("ParticleSystemComponent re-resolves its texture by asset UUID") {
  ensureEnv();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_particle_tex_test";
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
  ParticleSystemComponent* a = src->addComponent<ParticleSystemComponent>();
  REQUIRE(a != nullptr);

  EmitterConfig cfg = makeConfig();
  cfg.textureAssetId = id;
  a->setConfig(cfg);

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  SceneNode* dst = scene.createNode("dst");
  REQUIRE(dst != nullptr);
  ParticleSystemComponent* b = dst->addComponent<ParticleSystemComponent>();
  REQUIRE(b != nullptr);
  b->onDeserialize(blob);  // resolves the texture via the running AssetManager

  CHECK(b->getConfig().textureAssetId.toString() == id.toString());
  REQUIRE(b->getConfig().texture != nullptr);
  CHECK(b->getConfig().texture->getSize() == sf::Vector2u{8u, 8u});

  FileSystem::removeAll(dir);
}

TEST_CASE("ParticleSystemComponent survives a full SceneSerializer round-trip") {
  ensureEnv();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_particle_scene_test";
  FileSystem::removeAll(dir);

  const sfmx::UUID id = sfmx::UUID::createRandom();
  writeTextureAsset(dir, id);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<TextureCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  Scene src("src");
  SceneNode* node = src.createNode("emitter");
  REQUIRE(node != nullptr);
  ParticleSystemComponent* ps = node->addComponent<ParticleSystemComponent>();
  REQUIRE(ps != nullptr);
  EmitterConfig cfg = makeConfig();
  cfg.textureAssetId = id;
  ps->setConfig(cfg);
  ps->setSortMode(ParticleSortMode::kBackToFront);

  MemoryDataStream blob;
  REQUIRE(SceneSerializer::serialize(src, blob));
  blob.seek(0);

  Scene dst("dst");
  REQUIRE(SceneSerializer::deserialize(dst, blob));

  Vector<SceneNode*> found = dst.findNodesByName("emitter");
  REQUIRE(found.size() == 1u);
  ParticleSystemComponent* ps2 = found[0]->getComponent<ParticleSystemComponent>();
  REQUIRE(ps2 != nullptr);
  CHECK(ps2->getConfig().maxParticles == 64);
  CHECK(ps2->getConfig().blendMode == sf::BlendAdd);
  CHECK(ps2->getSortMode() == ParticleSortMode::kBackToFront);
  CHECK(ps2->getConfig().textureAssetId.toString() == id.toString());
  REQUIRE(ps2->getConfig().texture != nullptr);
  CHECK(ps2->getConfig().texture->getSize() == sf::Vector2u{8u, 8u});

  FileSystem::removeAll(dir);
}
