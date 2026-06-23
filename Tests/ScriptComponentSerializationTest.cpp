#include <doctest/doctest.h>

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"
#include "core/MemoryDataStream.h"
#include "core/FileSystem.h"
#include "scene/ComponentRegistry.h"
#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "scene/SceneSerializer.h"
#include "scene/ScriptComponent.h"
#include "scripts/ScriptEngine.h"
#include "input/InputSystem.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// M5.3 — ScriptComponent serialization. The only persistent state is the script
// name (a path); the bound sol function is rebuilt on load via the ScriptEngine.
// This also required a (SceneNode*) ctor so ComponentRegistry can recreate it.

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

// Writes a minimal valid Lua script (returns a per-frame function) to a temp dir
// and returns its absolute path. Caller removes the dir when done.
String
writeTempScript(const FileSystemPath& dir) {
  FileSystem::removeAll(dir);
  const FileSystemPath path = dir / "s.lua";
  const String body = "return function(self, dt) end\n";

  SPtr<DataStream> out = FileSystem::createAndOpenFile(path);
  REQUIRE(out != nullptr);
  out->write(body.data(), body.size());
  out->close();
  return path.string();
}

} // namespace

TEST_CASE("ScriptComponent round-trips the script name without the engine") {
  ensureEnv();

  Scene scene("s");
  SceneNode* src = scene.createNode("src");
  SceneNode* dst = scene.createNode("dst");
  REQUIRE(src != nullptr);
  REQUIRE(dst != nullptr);

  // Engine not required: the 2-arg ctor is guarded, so the name is set but no
  // binding happens. (A bogus path also stays uninitialized if the engine IS up.)
  ScriptComponent* a = src->addComponent<ScriptComponent>("scripts/does_not_exist.lua");
  REQUIRE(a != nullptr);
  CHECK(a->getScriptName() == "scripts/does_not_exist.lua");

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  ScriptComponent* b = dst->addComponent<ScriptComponent>();  // deferred ctor
  REQUIRE(b != nullptr);
  b->onDeserialize(blob);

  CHECK(b->getScriptName() == "scripts/does_not_exist.lua");
  CHECK_FALSE(b->isInitialized());  // unloadable path → never bound
}

TEST_CASE("ScriptComponent re-binds its Lua function on deserialize with the engine") {
  ensureEnv();
  ensureScriptEngine();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_script_test";
  const String scriptPath = writeTempScript(dir);

  Scene scene("s");
  SceneNode* src = scene.createNode("src");
  REQUIRE(src != nullptr);

  ScriptComponent* a = src->addComponent<ScriptComponent>(scriptPath);
  REQUIRE(a != nullptr);
  CHECK(a->isInitialized());  // engine up + valid script → bound at construction

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  SceneNode* dst = scene.createNode("dst");
  REQUIRE(dst != nullptr);
  ScriptComponent* b = dst->addComponent<ScriptComponent>();  // deferred
  REQUIRE(b != nullptr);
  REQUIRE_FALSE(b->isInitialized());
  b->onDeserialize(blob);

  CHECK(b->getScriptName() == scriptPath);
  CHECK(b->isInitialized());  // re-bound from the name via the ScriptEngine

  FileSystem::removeAll(dir);
}

TEST_CASE("ScriptComponent re-binds through a full SceneSerializer round-trip") {
  ensureEnv();
  ensureScriptEngine();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_script_scene_test";
  const String scriptPath = writeTempScript(dir);

  Scene src("src");
  SceneNode* node = src.createNode("brain");
  REQUIRE(node != nullptr);
  ScriptComponent* script = node->addComponent<ScriptComponent>(scriptPath);
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
  CHECK(script2->getScriptName() == scriptPath);
  CHECK(script2->isInitialized());

  FileSystem::removeAll(dir);
}
