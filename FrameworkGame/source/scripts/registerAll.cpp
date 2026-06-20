#include "core/platform/Prerequisites.h"

#include "scripts/registerAll.h"

#include "scripts/registerAngle.h"
#include "scripts/registerColor.h"
#include "scripts/registerVector2i.h"
#include "scripts/registerVector2f.h"
#include "scripts/registerVector3i.h"
#include "scripts/registerVector3f.h"
#include "scripts/registerIntRect.h"
#include "scripts/registerFloatRect.h"
#include "scripts/registerTransform.h"

#include "scripts/registerInputTypes.h"
#include "scripts/registerKeyboard.h"
#include "scripts/registerMouse.h"
#include "scripts/registerGamepad.h"

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
  // Value types.
  RegisterAngle(lua);
  RegisterColor(lua);
  RegisterVector2i(lua);
  RegisterVector2f(lua);
  RegisterVector3i(lua);
  RegisterVector3f(lua);
  RegisterIntRect(lua);
  RegisterFloatRect(lua);
  RegisterTransform(lua);

  // Input types.
  RegisterInputTypes(lua);
  RegisterKeyboard(lua);
  RegisterMouse(lua);
  RegisterGamepad(lua);
}

}

}