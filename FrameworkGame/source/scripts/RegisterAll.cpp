#include "scripts/RegisterAll.h"

#include "core/platform/Prerequisites.h"

#include "scripts/RegisterAngle.h"
#include "scripts/RegisterColor.h"
#include "scripts/RegisterVector2i.h"
#include "scripts/RegisterVector2f.h"
#include "scripts/RegisterVector3i.h"
#include "scripts/RegisterVector3f.h"
#include "scripts/RegisterIntRect.h"
#include "scripts/RegisterFloatRect.h"
#include "scripts/RegisterTransform.h"

#include "scripts/RegisterInputTypes.h"
#include "scripts/RegisterKeyboard.h"
#include "scripts/RegisterMouse.h"
#include "scripts/RegisterGamepad.h"

#include "scripts/RegisterComponent.h"
#include "scripts/RegisterComponentAccess.h"
#include "scripts/RegisterTransformComponent.h"

#include "scripts/RegisterScene.h"
#include "scripts/RegisterSceneNode.h"

#include "scene/SceneNode.h"
#include "scene/SourceComponent.h"

namespace sfmx
{

namespace script
{

void
registerAll(sol::state_view lua) {
  lua.open_libraries(sol::lib::base,
                     sol::lib::math,
                     sol::lib::string,
                     sol::lib::table);
  // Value types.
  registerAngle(lua);
  registerColor(lua);
  registerVector2i(lua);
  registerVector2f(lua);
  registerVector3i(lua);
  registerVector3f(lua);
  registerIntRect(lua);
  registerFloatRect(lua);
  registerTransform(lua);

  // Input types.
  registerInputTypes(lua);
  registerKeyboard(lua);
  registerMouse(lua);
  registerGamepad(lua);

  // Component hierarchy: the base must precede its derived usertypes so the
  // sol::bases<Component> links resolve.
  registerComponent(lua);
  registerTransformComponent(lua);

  // Scene graph.
  registerSceneNode(lua);
  registerScene(lua);
}

}  // namespace script

}  // namespace sfmx