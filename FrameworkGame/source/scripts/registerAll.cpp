#include "scripts/registerAll.h"

#include "core/platform/Prerequisites.h"

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

#include "scripts/registerComponent.h"
#include "scripts/registerComponentAccess.h"
#include "scripts/registerTransformComponent.h"

#include "scripts/registerScene.h"
#include "scripts/registerSceneNode.h"

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