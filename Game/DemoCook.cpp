#include "DemoCook.h"

#include "DemoScene.h"

#include "config/IniFile.h"
#include "core/FileSystem.h"
#include "scene/Scene.h"
#include "scene/SceneSerializer.h"
#include "utils/MemoryPoolHandler.h"
#include "assets/AssetManager.h"
#include "assets/TextureCodec.h"

#include <iostream>

namespace demo {

using namespace sfmx;

int
cookScene() {
  // Cooking runs from the repo root; point the content root at the repo's "Game"
  // dir so the same relative content paths (config/, assets/, resources/) that the
  // shipped game resolves next to the exe resolve here to the repo layout.
  FileSystem::setContentRoot("Game");

  IniFile config;
  config.loadAll({"config/Engine.ini", "config/Game.ini"});
  const float w = static_cast<float>(config.getUInt("Window", "Width", 800u));
  const float h = static_cast<float>(config.getUInt("Window", "Height", 600u));

  // Minimal startup: pools to allocate the scene + AssetManager to resolve the
  // textures by UUID. No window, no InputSystem/PhysicsSystem, no game loop. All
  // component ctors guard on isStarted(), so the absent systems are harmless.
  MemoryPoolHandler::startUp(4096);
  registerDemoPools(MemoryPoolHandler::instance());
  AssetManager::startUp();
  AssetManager::instance().registerCodec(MakeShared<TextureCodec>());
  AssetManager::instance().mount("assets");

  Scene scene("Main");
  buildDemoScene(scene, w, h);
  const bool ok = SceneSerializer::saveToFile(scene, kSceneFile);
  std::cout << "[CookScene] " << (ok ? "wrote " : "FAILED ") << kSceneFile << "\n";

  scene.clear();
  // Pools (and the pooled components holding asset refs) go before the assets.
  MemoryPoolHandler::shutDown();
  AssetManager::shutDown();
  return ok ? 0 : 1;
}

} // namespace demo
