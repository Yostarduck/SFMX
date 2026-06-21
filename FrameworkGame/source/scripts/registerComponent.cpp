#include "scripts/registerComponent.h"

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
// Complete type required: getOwner returns SceneNode*, so sol2 needs the full
// definition to push it.
#include "scene/SceneNode.h"

namespace sfmx
{

namespace script
{

void
registerComponent(sol::state_view lua) {
  lua.new_usertype<Component>("Component",
    sol::no_constructor,

    "getOwner", &Component::getOwner,
    "getTypeId", &Component::getTypeId
  );
}

}  // namespace script

}  // namespace sfmx