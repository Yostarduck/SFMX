#include "core/platform/Prerequisites.h"

#include "scripts/registerAll.h"

#include "scene/SceneNode.h"
#include "scene/SpriteComponent.h"

namespace sfmx
{

namespace script
{

void
RegisterAll(sol::state_view lua) {
  lua.open_libraries(sol::lib::base,
                     sol::lib::math,
                     sol::lib::string,
                     sol::lib::table);
}

}

}