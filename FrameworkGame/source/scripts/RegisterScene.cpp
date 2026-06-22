#include "scripts/RegisterScene.h"

#include "core/platform/Prerequisites.h"
#include "scene/Scene.h"

namespace sfmx
{

namespace script
{

void
registerScene(sol::state_view lua) {
  lua.new_usertype<Scene>("Scene",
    sol::no_constructor,

    "getRoot", &Scene::getRoot,
    "getName", &Scene::getName,

    "createNode", &Scene::createNode,

    "destroyNode", sol::overload(
      [](Scene& s, SceneNode* node) { s.destroyNode(node); },
      [](Scene& s, NodeId id) { s.destroyNode(id); }
    ),

    "findNode", &Scene::findNode,

    "getNodeCount", &Scene::getNodeCount,

    "setCamera", &Scene::setCamera,
    "getCamera", &Scene::getCamera,
    "addCamera", &Scene::addCamera,
    "removeCamera", &Scene::removeCamera,
    "clearCameras", &Scene::clearCameras,
    "getCameraCount", &Scene::getCameraCount,

    "transform", [](SceneNode& n) -> Transform& { return n.transform(); }
  );
}

}  // namespace script

}  // namespace sfmx