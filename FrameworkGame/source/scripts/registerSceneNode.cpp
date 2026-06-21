#include "scripts/registerSceneNode.h"

#include "core/platform/Prerequisites.h"
#include "scene/SceneNode.h"

#include "scene/Transform.h"
#include <SFML/Graphics/Transform.hpp>

#include "scripts/registerComponentAccess.h"

namespace sfmx
{

namespace script
{

void
registerSceneNode(sol::state_view lua) {
  lua.new_usertype<SceneNode>("SceneNode",
    sol::no_constructor,

    "getId", &SceneNode::getId,
    "getName", &SceneNode::getName,
    "setName", &SceneNode::setName,
    "isEnabled", &SceneNode::isEnabled,
    "setEnabled", &SceneNode::setEnabled,
    "isVisible", &SceneNode::isVisible,
    "setVisible", &SceneNode::setVisible,

    "isEnabledInHierarchy", &SceneNode::isEnabledInHierarchy,

    "transform", [](SceneNode& n) -> Transform& { return n.transform(); },

    "getWorldTransform", &SceneNode::getWorldTransform,

    "getParent", &SceneNode::getParent,
    "getFirstChild", &SceneNode::getFirstChild,
    "getLastChild", &SceneNode::getLastChild,
    "getNextSibling", &SceneNode::getNextSibling,
    "getPrevSibling", &SceneNode::getPrevSibling,

    "getChildCount", &SceneNode::getChildCount,

    "isAncestorOf", &SceneNode::isAncestorOf,

    "findChild", &SceneNode::findChild,

    "createChild", &SceneNode::createChild,

    "reparent", sol::overload(
      [](SceneNode& n, SceneNode* parent) { n.reparent(parent); },
      [](SceneNode& n, SceneNode* parent, bool keep) { n.reparent(parent, keep); }
    ),

    "detachFromParent", &SceneNode::detachFromParent,

    // Generic, type-driven component access. The argument is a component
    // usertype table (e.g. SpriteComponent); dispatch is keyed by its typeId.
    "addComponent", &luaAddComponent,
    "getComponent", &luaGetComponent,
    "hasComponent", &luaHasComponent,
    "removeComponent", &luaRemoveComponent
  );
}

}  // namespace script

}  // namespace sfmx