#include <doctest/doctest.h>

#include <cstdio>

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"
#include "core/MemoryDataStream.h"
#include "core/FileSystem.h"
#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include "assets/LuaAsset.h"
#include "assets/LuaCodec.h"
#include "scene/ComponentRegistry.h"
#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "scene/SceneSerializer.h"
#include "scene/ScriptComponent.h"
#include "scripts/ScriptEngine.h"
#include "input/InputSystem.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// sfmx::UUID is qualified because on Windows <rpcdce.h> (via stduuid) defines a
// global ::UUID. LuaAsset migration: ScriptComponent now serializes a LuaAsset
// UUID (was a path); the sol function is rebuilt on load from the asset's text.

namespace {

void
ensureEnv() {
  if (!MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::startUp(4096);
  }
  MemoryPoolHandler& pools = MemoryPoolHandler::instance();
  if (!pools.hasPool<SceneNode>()) {
    pools.registerPool<SceneNode>(256);
  }
  if (!pools.hasPool<ScriptComponent>()) {
    pools.registerPool<ScriptComponent>(64);
  }
  if (!ComponentRegistry::isStarted()) {
    ComponentRegistry::startUp();
  }
  ComponentRegistry::instance().registerComponent<ScriptComponent>();
}

// The ScriptEngine binds Keyboard/Mouse/Gamepad on startup, so InputSystem (which
// starts those devices) must be up first — same order as the game's bootstrap.
void
ensureScriptEngine() {
  if (!InputSystem::isStarted()) {
    InputSystem::startUp();
  }
  if (!ScriptEngine::isStarted()) {
    ScriptEngine::startUp();
  }
}

// RAII: clean startUp/shutDown of the AssetManager even if a REQUIRE throws.
struct ManagerScope {
  ManagerScope() {
    if (AssetManager::isStarted()) { AssetManager::shutDown(); }
    AssetManager::startUp();
  }
  ~ManagerScope() {
    if (AssetManager::isStarted()) { AssetManager::shutDown(); }
  }
};

// Cook a minimal valid Lua script (returns a per-frame function) into a LuaAsset
// `.sfmxasset` with the given id, inside dir.
void
writeLuaAsset(const FileSystemPath& dir, const sfmx::UUID& id) {
  const String body = "return function(self, dt) end\n";

  AssetFileWriter writer;
  AssetMetadata meta;
  meta.uuid      = id;
  meta.assetType = TypeTraits<LuaAsset>::getTypeId();
  std::snprintf(meta.name, sizeof(meta.name), "%s", "script");
  writer.setMetadata(meta);
  writer.addChunk(body.data(), body.size(), ChunkFormat::kRaw);

  SPtr<DataStream> out = FileSystem::createAndOpenFile(dir / "script.sfmxasset");
  REQUIRE(out != nullptr);
  REQUIRE(writer.writeTo(*out));
  out->close();
}

} // namespace

TEST_CASE("ScriptComponent round-trips the script UUID without resolving") {
  ensureEnv();

  Scene scene("s");
  SceneNode* src = scene.createNode("src");
  SceneNode* dst = scene.createNode("dst");
  REQUIRE(src != nullptr);
  REQUIRE(dst != nullptr);

  // A random, un-cataloged id never resolves → no binding, but the id is kept so
  // it still re-serializes (stays GL/engine-free regardless).
  const sfmx::UUID id = sfmx::UUID::createRandom();
  ScriptComponent* a = src->addComponent<ScriptComponent>(id);
  REQUIRE(a != nullptr);
  CHECK(a->getScriptAssetId().toString() == id.toString());
  CHECK_FALSE(a->isInitialized());

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  ScriptComponent* b = dst->addComponent<ScriptComponent>();  // deferred ctor
  REQUIRE(b != nullptr);
  b->onDeserialize(blob);

  CHECK(b->getScriptAssetId().toString() == id.toString());
  CHECK_FALSE(b->isInitialized());
}

TEST_CASE("ScriptComponent re-binds its Lua function from a cataloged LuaAsset") {
  ensureEnv();
  ensureScriptEngine();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_script_test";
  FileSystem::removeAll(dir);
  const sfmx::UUID id = sfmx::UUID::createRandom();
  writeLuaAsset(dir, id);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<LuaCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  Scene scene("s");
  SceneNode* src = scene.createNode("src");
  REQUIRE(src != nullptr);

  ScriptComponent* a = src->addComponent<ScriptComponent>(id);
  REQUIRE(a != nullptr);
  CHECK(a->isInitialized());  // engine up + resolved LuaAsset → bound at construction

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  SceneNode* dst = scene.createNode("dst");
  REQUIRE(dst != nullptr);
  ScriptComponent* b = dst->addComponent<ScriptComponent>();  // deferred
  REQUIRE(b != nullptr);
  REQUIRE_FALSE(b->isInitialized());
  b->onDeserialize(blob);

  CHECK(b->getScriptAssetId().toString() == id.toString());
  CHECK(b->isInitialized());  // re-bound from the LuaAsset via the ScriptEngine

  FileSystem::removeAll(dir);
}

TEST_CASE("ScriptComponent re-binds through a full SceneSerializer round-trip") {
  ensureEnv();
  ensureScriptEngine();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_script_scene_test";
  FileSystem::removeAll(dir);
  const sfmx::UUID id = sfmx::UUID::createRandom();
  writeLuaAsset(dir, id);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<LuaCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  Scene src("src");
  SceneNode* node = src.createNode("brain");
  REQUIRE(node != nullptr);
  ScriptComponent* script = node->addComponent<ScriptComponent>(id);
  REQUIRE(script != nullptr);
  REQUIRE(script->isInitialized());

  MemoryDataStream blob;
  REQUIRE(SceneSerializer::serialize(src, blob));
  blob.seek(0);

  Scene dst("dst");
  REQUIRE(SceneSerializer::deserialize(dst, blob));

  Vector<SceneNode*> found = dst.findNodesByName("brain");
  REQUIRE(found.size() == 1u);
  ScriptComponent* script2 = found[0]->getComponent<ScriptComponent>();
  REQUIRE(script2 != nullptr);
  CHECK(script2->getScriptAssetId().toString() == id.toString());
  CHECK(script2->isInitialized());

  FileSystem::removeAll(dir);
}
