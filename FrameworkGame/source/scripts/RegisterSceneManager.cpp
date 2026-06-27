#include "scripts/RegisterSceneManager.h"

#include "core/platform/Prerequisites.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"

namespace sfmx
{

namespace script
{

void
registerSceneManager(sol::state_view lua) {
  lua.new_usertype<SceneManager>("SceneManager",
    sol::no_constructor,
    
    "createScene", &SceneManager::createScene,

    "loadScene", sol::resolve<Scene*(StringView)>(&SceneManager::loadScene),

    "unloadScene", sol::overload(
      [](SceneManager& s, StringView name) { s.unloadScene(name); },
      [](SceneManager& s, Scene* scene) { s.unloadScene(scene); }
    ),
    "destroyScene", sol::overload(
      [](SceneManager& s, StringView name) { s.destroyScene(name); },
      [](SceneManager& s, Scene* scene) { s.destroyScene(scene); }
    ),

    "destroyAllScenes", &SceneManager::destroyAllScenes,
    "getActiveScene", &SceneManager::getActiveScene,
    "setActiveScene", sol::overload(
      [](SceneManager& s, StringView name) { s.setActiveScene(name); },
      [](SceneManager& s, Scene* scene) { s.setActiveScene(scene); }
    ),
    "findScene", &SceneManager::findScene,
    "hasScene", &SceneManager::hasScene,
    "isActive", &SceneManager::isActive,
    "getSceneCount", &SceneManager::getSceneCount
  );
  
  lua["SceneManager"] = std::ref(SceneManager::instance());
}

}  // namespace script

}  // namespace sfmx