#include <doctest/doctest.h>

#include <cstdio>
#include <variant>

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
#include "resource/Frame.h"
#include "scene/Animation.h"
#include "scene/AnimatorComponent.h"
#include "scene/ComponentRegistry.h"
#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "scene/SceneSerializer.h"
#include "scene/SpriteComponent.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// sfmx::UUID is qualified because on Windows <rpcdce.h> (via stduuid) defines a
// global ::UUID. M5.3: AnimatorComponent serializes its animation graph and rebinds
// each clip's shared atlas through the TextureAsset asset-handle convention; the
// current clip is re-selected stopped at t=0 (transient play state is not restored).

namespace {

// Idempotent, never-shutdown setup (suite convention — other suites share the
// global MemoryPoolHandler, so we never tear it down here). AnimatorComponent's
// ctor ensures a sibling SpriteComponent, so both pools must exist.
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
  if (!pools.hasPool<AnimatorComponent>()) {
    pools.registerPool<AnimatorComponent>(64);
  }

  if (!ComponentRegistry::isStarted()) {
    ComponentRegistry::startUp();
  }
  ComponentRegistry::instance().registerComponent<SpriteComponent>();
  ComponentRegistry::instance().registerComponent<AnimatorComponent>();
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

// Write an 8x8 blue PNG as a `.sfmxasset` TextureAsset (the shared atlas) with the id.
void
writeAtlasAsset(const FileSystemPath& dir, const sfmx::UUID& id) {
  sf::Image image(sf::Vector2u{8u, 8u}, sf::Color::Blue);
  Optional<Vector<uint8>> png = image.saveToMemory("png");
  REQUIRE(png.has_value());

  AssetFileWriter writer;
  AssetMetadata meta;
  meta.uuid      = id;
  meta.assetType = TypeTraits<TextureAsset>::getTypeId();
  std::snprintf(meta.name, sizeof(meta.name), "%s", "atlas");
  writer.setMetadata(meta);
  writer.addChunk(png->data(), png->size(), ChunkFormat::kPng);

  SPtr<DataStream> out = FileSystem::createAndOpenFile(dir / "atlas.sfmxasset");
  REQUIRE(out != nullptr);
  REQUIRE(writer.writeTo(*out));
  out->close();
}

// Build a clip with frames carrying only framing rects (texture is bound on load
// from the atlas UUID). frameDurations stays empty unless set by the caller.
SPtr<Animation>
makeClip(bool loops, float speed, float duration, const sfmx::UUID& atlas,
         const Vector<sf::IntRect>& rects) {
  auto anim = MakeShared<Animation>();
  anim->m_loops           = loops;
  anim->m_speedMultiplier = speed;
  anim->m_duration        = duration;
  anim->m_textureAssetId  = atlas;
  for (const sf::IntRect& r : rects) {
    Frame frame;
    frame.framing = r;
    anim->m_frames.push_back(frame);
  }
  return anim;
}

} // namespace

TEST_CASE("AnimatorComponent round-trips the animation graph, transitions and params (no GL)") {
  ensureEnv();

  Scene scene("s");
  SceneNode* src = scene.createNode("src");
  REQUIRE(src != nullptr);

  AnimatorComponent* a = src->addComponent<AnimatorComponent>();
  REQUIRE(a != nullptr);

  // Two clips; atlas UUID null → frames keep null textures (stays GL-free).
  SPtr<Animation> idle =
      makeClip(true, 1.0f, 2.0f, sfmx::UUID::null(),
               {sf::IntRect({0, 0}, {16, 16}), sf::IntRect({16, 0}, {16, 16})});
  idle->m_frameDurations = {0.5f, 0.5f};
  SPtr<Animation> walk =
      makeClip(false, 2.0f, 0.8f, sfmx::UUID::null(), {sf::IntRect({0, 0}, {16, 16})});

  a->addAnimation(idle, "idle");
  a->addAnimation(walk, "walk");

  Map<String, Param> cond;
  cond["moving"] = {ParamType::kBool, true};
  a->addTransition("idle", "walk", true, true, cond);

  a->setBool("moving", true);
  a->setFloat("speed", 3.5f);
  a->setInt("hp", 7);
  a->setTrigger("jump");
  // No clip selected (avoids setFrame on textureless frames) → current stays null.

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  SceneNode* dst = scene.createNode("dst");
  REQUIRE(dst != nullptr);
  AnimatorComponent* b = dst->addComponent<AnimatorComponent>();
  REQUIRE(b != nullptr);
  b->onDeserialize(blob);

  CHECK(b->animationExists("idle"));
  CHECK(b->animationExists("walk"));

  const auto& anims = b->getAnimations();
  REQUIRE(anims.count("idle") == 1);
  const AnimationNode* idleNode = anims.at("idle").get();
  REQUIRE(idleNode->animation != nullptr);
  CHECK(idleNode->animation->m_loops == true);
  CHECK(idleNode->animation->m_speedMultiplier == doctest::Approx(1.0f));
  CHECK(idleNode->animation->m_duration == doctest::Approx(2.0f));
  REQUIRE(idleNode->animation->m_frames.size() == 2);
  CHECK(idleNode->animation->m_frames[1].framing.position.x == 16);
  CHECK(idleNode->animation->m_frames[1].framing.size.y == 16);
  REQUIRE(idleNode->animation->m_frameDurations.size() == 2);
  CHECK(idleNode->animation->m_frameDurations[0] == doctest::Approx(0.5f));

  REQUIRE(idleNode->transitions.size() == 1);
  CHECK(idleNode->transitions[0]->exit == "walk");
  CHECK(idleNode->transitions[0]->hasExitTime == true);
  CHECK(idleNode->transitions[0]->shouldTransition == true);
  REQUIRE(idleNode->transitions[0]->params.count("moving") == 1);
  CHECK(idleNode->transitions[0]->params.at("moving").type == ParamType::kBool);

  const AnimationNode* walkNode = anims.at("walk").get();
  REQUIRE(walkNode->animation != nullptr);
  CHECK(walkNode->animation->m_loops == false);
  CHECK(walkNode->animation->m_speedMultiplier == doctest::Approx(2.0f));
  CHECK(walkNode->transitions.empty());

  const auto& params = b->getParams();
  REQUIRE(params.count("moving") == 1);
  CHECK(params.at("moving").type == ParamType::kBool);
  CHECK(std::get<bool>(params.at("moving").value) == true);
  REQUIRE(params.count("speed") == 1);
  CHECK(params.at("speed").type == ParamType::kFloat);
  CHECK(std::get<float>(params.at("speed").value) == doctest::Approx(3.5f));
  REQUIRE(params.count("hp") == 1);
  CHECK(params.at("hp").type == ParamType::kInt);
  CHECK(std::get<int>(params.at("hp").value) == 7);
  REQUIRE(params.count("jump") == 1);
  CHECK(params.at("jump").type == ParamType::kTrigger);

  CHECK(b->getState() == AnimationState::kStopped);
  CHECK(b->getCurrentAnimation() == nullptr);
}

TEST_CASE("AnimatorComponent re-resolves each clip's atlas texture by UUID") {
  ensureEnv();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_anim_atlas_test";
  FileSystem::removeAll(dir);

  const sfmx::UUID id = sfmx::UUID::createRandom();
  writeAtlasAsset(dir, id);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<TextureCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  Scene scene("s");
  SceneNode* src = scene.createNode("src");
  REQUIRE(src != nullptr);
  AnimatorComponent* a = src->addComponent<AnimatorComponent>();
  REQUIRE(a != nullptr);

  SPtr<Animation> run =
      makeClip(true, 1.0f, 0.4f, id,
               {sf::IntRect({0, 0}, {4, 4}), sf::IntRect({4, 0}, {4, 4})});
  a->addAnimation(run, "run");

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  SceneNode* dst = scene.createNode("dst");
  REQUIRE(dst != nullptr);
  AnimatorComponent* b = dst->addComponent<AnimatorComponent>();
  REQUIRE(b != nullptr);
  b->onDeserialize(blob);  // resolves the atlas via the running AssetManager

  const auto& anims = b->getAnimations();
  REQUIRE(anims.count("run") == 1);
  const AnimationNode* runNode = anims.at("run").get();
  REQUIRE(runNode->animation != nullptr);
  CHECK(runNode->animation->m_textureAssetId.toString() == id.toString());
  REQUIRE(runNode->animation->m_frames.size() == 2);
  CHECK(runNode->animation->m_frames[0].texture != nullptr);
  CHECK(runNode->animation->m_frames[1].texture != nullptr);
  // All frames share the one atlas texture.
  CHECK(runNode->animation->m_frames[0].texture == runNode->animation->m_frames[1].texture);
  CHECK(runNode->animation->m_frames[1].framing.position.x == 4);

  FileSystem::removeAll(dir);
}

TEST_CASE("AnimatorComponent param variant round-trips every ParamType") {
  ensureEnv();

  Scene scene("s");
  SceneNode* src = scene.createNode("src");
  REQUIRE(src != nullptr);
  AnimatorComponent* a = src->addComponent<AnimatorComponent>();
  REQUIRE(a != nullptr);

  a->setBool("b", false);
  a->setFloat("f", -12.25f);
  a->setInt("i", -3);
  a->setTrigger("t");

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  SceneNode* dst = scene.createNode("dst");
  REQUIRE(dst != nullptr);
  AnimatorComponent* b = dst->addComponent<AnimatorComponent>();
  REQUIRE(b != nullptr);
  b->onDeserialize(blob);

  const auto& params = b->getParams();
  REQUIRE(params.count("b") == 1);
  CHECK(params.at("b").type == ParamType::kBool);
  CHECK(std::get<bool>(params.at("b").value) == false);
  REQUIRE(params.count("f") == 1);
  CHECK(params.at("f").type == ParamType::kFloat);
  CHECK(std::get<float>(params.at("f").value) == doctest::Approx(-12.25f));
  REQUIRE(params.count("i") == 1);
  CHECK(params.at("i").type == ParamType::kInt);
  CHECK(std::get<int>(params.at("i").value) == -3);
  REQUIRE(params.count("t") == 1);
  CHECK(params.at("t").type == ParamType::kTrigger);
}

TEST_CASE("AnimatorComponent survives a full SceneSerializer round-trip") {
  ensureEnv();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_anim_scene_test";
  FileSystem::removeAll(dir);

  const sfmx::UUID id = sfmx::UUID::createRandom();
  writeAtlasAsset(dir, id);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<TextureCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  Scene src("src");
  SceneNode* node = src.createNode("hero");
  REQUIRE(node != nullptr);
  // Sprite first so the animator ctor binds to it deterministically.
  node->addComponent<SpriteComponent>();
  AnimatorComponent* anim = node->addComponent<AnimatorComponent>();
  REQUIRE(anim != nullptr);

  SPtr<Animation> run = makeClip(true, 1.0f, 0.2f, id, {sf::IntRect({0, 0}, {8, 8})});
  anim->addAnimation(run, "run");

  MemoryDataStream blob;
  REQUIRE(SceneSerializer::serialize(src, blob));
  blob.seek(0);

  Scene dst("dst");
  REQUIRE(SceneSerializer::deserialize(dst, blob));

  Vector<SceneNode*> found = dst.findNodesByName("hero");
  REQUIRE(found.size() == 1u);
  AnimatorComponent* anim2 = found[0]->getComponent<AnimatorComponent>();
  REQUIRE(anim2 != nullptr);
  CHECK(anim2->animationExists("run"));

  const auto& anims = anim2->getAnimations();
  REQUIRE(anims.count("run") == 1);
  const SPtr<Animation>& a2 = anims.at("run")->animation;
  REQUIRE(a2 != nullptr);
  REQUIRE(a2->m_frames.size() == 1);
  CHECK(a2->m_frames[0].texture != nullptr);  // atlas resolved on scene load
  CHECK(a2->m_frames[0].framing.size.x == 8);

  FileSystem::removeAll(dir);
}
