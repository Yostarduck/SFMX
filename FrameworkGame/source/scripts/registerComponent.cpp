#include "core/platform/Prerequisites.h"
#include "scripts/registerComponent.h"

#include "scene/Component.h"
// Complete type required: getOwner returns SceneNode*, so sol2 needs the full
// definition to push it.
#include "scene/SceneNode.h"

namespace sfmx
{

namespace script
{

void
RegisterComponent(sol::state_view lua) {
  lua.new_usertype<Component>("Component",
    sol::no_constructor,

    "getOwner", &Component::getOwner,
    "getTypeId", &Component::getTypeId
  );
}

}

}