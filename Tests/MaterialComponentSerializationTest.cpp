#include <doctest/doctest.h>

#include <SFML/Graphics/Color.hpp>

#include "assets/MaterialData.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"
#include "core/MemoryDataStream.h"
#include "core/platform/Prerequisites.h"
#include "scene/ComponentRegistry.h"
#include "scene/MaterialComponent.h"
#include "scene/ParticleSystemComponent.h"
#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "scene/SceneSerializer.h"
#include "scene/SpriteComponent.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// MaterialComponent stores a shared MaterialData payload: shader by asset UUID,
// the uniform set, and the sampler settings. These tests cover the payload
// round-trip, the component round-trip (without a running AssetManager), and a
// full scene round-trip where drawables relink their sibling material.

namespace {

// Idempotent, never-shutdown setup (suite convention — other suites share the
// global MemoryPoolHandler).
void
ensureEnv() {
  if (!MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::startUp(4096);
  }
  MemoryPoolHandler& pools = MemoryPoolHandler::instance();
  if (!pools.hasPool<SceneNode>()) {
    pools.registerPool<SceneNode>(256);
  }
  if (!pools.hasPool<MaterialComponent>()) {
    pools.registerPool<MaterialComponent>(64);
  }
  if (!pools.hasPool<SpriteComponent>()) {
    pools.registerPool<SpriteComponent>(64);
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
  ComponentRegistry& registry = ComponentRegistry::instance();
  registry.registerComponent<MaterialComponent>();
  registry.registerComponent<SpriteComponent>();
  registry.registerComponent<ParticleSystemComponent>();
}

} // namespace

TEST_CASE("MaterialData round-trips shader id, uniforms and sampler settings") {
  MaterialData source;
  source.shaderAssetId = UUID::createRandom();
  source.textureUniform = "u_albedo";
  source.bindCurrentTexture = false;

  MaterialUniform intensity;
  intensity.name = "u_intensity";
  intensity.kind = MaterialUniformKind::kFloat;
  intensity.value[0] = 0.75f;
  source.uniforms.push_back(intensity);

  MaterialUniform offset;
  offset.name = "u_offset";
  offset.kind = MaterialUniformKind::kVec2;
  offset.value[0] = 1.25f;
  offset.value[1] = -2.5f;
  source.uniforms.push_back(offset);

  MaterialUniform tint;
  tint.name = "u_tint";
  tint.kind = MaterialUniformKind::kVec3;
  tint.value[0] = 0.1f;
  tint.value[1] = 0.2f;
  tint.value[2] = 0.3f;
  source.uniforms.push_back(tint);

  MaterialUniform color;
  color.name = "u_color";
  color.kind = MaterialUniformKind::kVec4;
  color.value[0] = 1.f;
  color.value[1] = 0.5f;
  color.value[2] = 0.f;
  color.value[3] = 0.25f;
  source.uniforms.push_back(color);

  MemoryDataStream blob;
  source.serialize(blob);
  blob.seek(0);

  MaterialData restored;
  REQUIRE(restored.deserialize(blob));

  CHECK(restored.shaderAssetId == source.shaderAssetId);
  CHECK(restored.textureUniform == "u_albedo");
  CHECK_FALSE(restored.bindCurrentTexture);
  REQUIRE(restored.uniforms.size() == 4u);

  CHECK(restored.uniforms[0].name == "u_intensity");
  CHECK(restored.uniforms[0].kind == MaterialUniformKind::kFloat);
  CHECK(restored.uniforms[0].value[0] == doctest::Approx(0.75f));

  CHECK(restored.uniforms[1].name == "u_offset");
  CHECK(restored.uniforms[1].kind == MaterialUniformKind::kVec2);
  CHECK(restored.uniforms[1].value[0] == doctest::Approx(1.25f));
  CHECK(restored.uniforms[1].value[1] == doctest::Approx(-2.5f));

  CHECK(restored.uniforms[2].name == "u_tint");
  CHECK(restored.uniforms[2].kind == MaterialUniformKind::kVec3);
  CHECK(restored.uniforms[2].value[2] == doctest::Approx(0.3f));

  CHECK(restored.uniforms[3].name == "u_color");
  CHECK(restored.uniforms[3].kind == MaterialUniformKind::kVec4);
  CHECK(restored.uniforms[3].value[1] == doctest::Approx(0.5f));
  CHECK(restored.uniforms[3].value[3] == doctest::Approx(0.25f));
}

TEST_CASE("MaterialData rejects an unknown version and an over-budget count") {
  {
    MemoryDataStream blob;
    blob << static_cast<uint32>(MaterialData::kVersion + 1);
    blob.seek(0);
    MaterialData data;
    CHECK_FALSE(data.deserialize(blob));
  }

  {
    MemoryDataStream blob;
    blob << MaterialData::kVersion;
    blob << UUID::null();
    blob << static_cast<uint64>(MaterialData::kMaxUniforms + 1);
    blob.seek(0);
    MaterialData data;
    CHECK_FALSE(data.deserialize(blob));
  }
}

TEST_CASE("MaterialComponent round-trips its payload (no GL, no AssetManager)") {
  ensureEnv();

  Scene scene("s");
  SceneNode* src = scene.createNode("src");
  SceneNode* dst = scene.createNode("dst");
  REQUIRE(src != nullptr);
  REQUIRE(dst != nullptr);

  MaterialComponent* a = src->addComponent<MaterialComponent>();
  REQUIRE(a != nullptr);

  // A random, un-cataloged id never resolves; the id must still round-trip.
  const UUID shaderId = UUID::createRandom();
  a->setShaderAssetId(shaderId);
  CHECK(a->getShader() == nullptr);

  a->setFloat("u_intensity", 0.6f);
  a->setVec2("u_offset", 3.f, 4.f);
  a->setVec3("u_tint", 0.1f, 0.2f, 0.3f);
  a->setColor("u_color", sf::Color(255, 128, 0, 64));
  a->setTextureUniform("u_albedo");
  a->setBindCurrentTexture(false);

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  MaterialComponent* b = dst->addComponent<MaterialComponent>();
  REQUIRE(b != nullptr);
  b->onDeserialize(blob);

  CHECK(b->getShaderAssetId() == shaderId);
  CHECK(b->getShader() == nullptr);
  CHECK(b->getTextureUniform() == "u_albedo");
  CHECK_FALSE(b->getBindCurrentTexture());

  const MaterialData& data = b->getData();
  REQUIRE(data.uniforms.size() == 4u);

  CHECK(data.uniforms[0].name == "u_intensity");
  CHECK(data.uniforms[0].kind == MaterialUniformKind::kFloat);
  CHECK(data.uniforms[0].value[0] == doctest::Approx(0.6f));

  CHECK(data.uniforms[1].kind == MaterialUniformKind::kVec2);
  CHECK(data.uniforms[1].value[0] == doctest::Approx(3.f));
  CHECK(data.uniforms[1].value[1] == doctest::Approx(4.f));

  CHECK(data.uniforms[2].kind == MaterialUniformKind::kVec3);
  CHECK(data.uniforms[2].value[2] == doctest::Approx(0.3f));

  CHECK(data.uniforms[3].kind == MaterialUniformKind::kVec4);
  CHECK(data.uniforms[3].value[0] == doctest::Approx(1.f));
  CHECK(data.uniforms[3].value[1] == doctest::Approx(128.f / 255.f));
  CHECK(data.uniforms[3].value[2] == doctest::Approx(0.f));
  CHECK(data.uniforms[3].value[3] == doctest::Approx(64.f / 255.f));
}

TEST_CASE("MaterialComponent keeps its state when the payload version is unknown") {
  ensureEnv();

  Scene scene("s");
  SceneNode* node = scene.createNode("node");
  REQUIRE(node != nullptr);
  MaterialComponent* material = node->addComponent<MaterialComponent>();
  REQUIRE(material != nullptr);
  material->setFloat("u_intensity", 0.5f);

  MemoryDataStream blob;
  blob << static_cast<uint32>(MaterialData::kVersion + 1);
  blob.seek(0);
  material->onDeserialize(blob);

  // The bad payload is rejected without clearing the existing uniform set.
  REQUIRE(material->getData().uniforms.size() == 1u);
  CHECK(material->getData().uniforms[0].value[0] == doctest::Approx(0.5f));
}

TEST_CASE("A sprite relinks its sibling material after a scene round-trip") {
  ensureEnv();

  Scene src("src");
  SceneNode* node = src.createNode("billboard");
  REQUIRE(node != nullptr);

  // Sprite first, material second: the material does not exist yet when the
  // sprite deserializes, so only the post-deserialize pass can relink it.
  SpriteComponent* sprite = node->addComponent<SpriteComponent>();
  REQUIRE(sprite != nullptr);
  CHECK(sprite->getMaterial() == nullptr);

  MaterialComponent* material = node->addComponent<MaterialComponent>();
  REQUIRE(material != nullptr);
  const UUID shaderId = UUID::createRandom();
  material->setShaderAssetId(shaderId);
  material->setFloat("u_intensity", 0.9f);

  MemoryDataStream blob;
  REQUIRE(SceneSerializer::serialize(src, blob));
  blob.seek(0);

  Scene dst("dst");
  REQUIRE(SceneSerializer::deserialize(dst, blob));

  Vector<SceneNode*> found = dst.findNodesByName("billboard");
  REQUIRE(found.size() == 1u);
  SceneNode* loaded = found[0];
  SpriteComponent* loadedSprite = loaded->getComponent<SpriteComponent>();
  MaterialComponent* loadedMaterial = loaded->getComponent<MaterialComponent>();
  REQUIRE(loadedSprite != nullptr);
  REQUIRE(loadedMaterial != nullptr);

  CHECK(loadedMaterial->getShaderAssetId() == shaderId);
  REQUIRE(loadedMaterial->getData().uniforms.size() == 1u);
  CHECK(loadedMaterial->getData().uniforms[0].value[0] == doctest::Approx(0.9f));

  // The relink happened even though the material was serialized after the sprite.
  CHECK(loadedSprite->getMaterial() == loadedMaterial);
}

TEST_CASE("A particle system relinks its sibling material after a scene round-trip") {
  ensureEnv();

  Scene src("src");
  SceneNode* node = src.createNode("emitter");
  REQUIRE(node != nullptr);

  ParticleSystemComponent* particles =
      node->addComponent<ParticleSystemComponent>();
  REQUIRE(particles != nullptr);
  CHECK(particles->getMaterial() == nullptr);

  MaterialComponent* material = node->addComponent<MaterialComponent>();
  REQUIRE(material != nullptr);
  material->setBindCurrentTexture(false);

  MemoryDataStream blob;
  REQUIRE(SceneSerializer::serialize(src, blob));
  blob.seek(0);

  Scene dst("dst");
  REQUIRE(SceneSerializer::deserialize(dst, blob));

  Vector<SceneNode*> found = dst.findNodesByName("emitter");
  REQUIRE(found.size() == 1u);
  SceneNode* loaded = found[0];
  ParticleSystemComponent* loadedParticles =
      loaded->getComponent<ParticleSystemComponent>();
  MaterialComponent* loadedMaterial = loaded->getComponent<MaterialComponent>();
  REQUIRE(loadedParticles != nullptr);
  REQUIRE(loadedMaterial != nullptr);

  CHECK(loadedParticles->getMaterial() == loadedMaterial);
  CHECK_FALSE(loadedMaterial->getBindCurrentTexture());
}

TEST_CASE("A material with no drawable sibling round-trips on its own") {
  ensureEnv();

  Scene src("src");
  SceneNode* node = src.createNode("standalone");
  REQUIRE(node != nullptr);
  MaterialComponent* material = node->addComponent<MaterialComponent>();
  REQUIRE(material != nullptr);
  material->setVec2("u_offset", 7.f, 8.f);

  MemoryDataStream blob;
  REQUIRE(SceneSerializer::serialize(src, blob));
  blob.seek(0);

  Scene dst("dst");
  REQUIRE(SceneSerializer::deserialize(dst, blob));

  Vector<SceneNode*> found = dst.findNodesByName("standalone");
  REQUIRE(found.size() == 1u);
  MaterialComponent* loaded = found[0]->getComponent<MaterialComponent>();
  REQUIRE(loaded != nullptr);
  REQUIRE(loaded->getData().uniforms.size() == 1u);
  CHECK(loaded->getData().uniforms[0].value[0] == doctest::Approx(7.f));
  CHECK(loaded->getData().uniforms[0].value[1] == doctest::Approx(8.f));
}
