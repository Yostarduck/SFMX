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
#include "scripts/RegisterSourceComponent.h"
#include "scripts/RegisterListenerComponent.h"
#include "scripts/RegisterCameraComponent.h"
#include "scripts/RegisterSpriteComponent.h"
#include "scripts/RegisterAnimatorComponent.h"
//#include "scripts/RegisterParticleSystemComponent.h"
#include "scripts/RegisterScriptComponent.h"

#include "scripts/RegisterScene.h"
#include "scripts/RegisterSceneNode.h"

#include "scene/SceneNode.h"
#include "scene/SourceComponent.h"
#include "scene/ListenerComponent.h"
#include "scene/CameraComponent.h"
#include "scene/SpriteComponent.h"
#include "scene/AnimatorComponent.h"
#include "scene/ParticleSystemComponent.h"
#include "scene/ScriptComponent.h"

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
  registerSourceComponent(lua);
  registerListenerComponent(lua);
  registerCameraComponent(lua);
  registerSpriteComponent(lua);
  registerAnimatorComponent(lua);
  //registerParticleSystemComponent(lua); // Not ready yet, wait for ResourceManager
  registerScriptComponent(lua);

  // Scene graph.
  registerSceneNode(lua);
  registerScene(lua);

  // Type-driven component access (node:addComponent(SpriteComponent), ...).
  // Register one entry per pool-allocated component type; the inline Transform
  // is reached through node:transform() and is intentionally not listed here.
  registerComponentType<SourceComponent>();
  registerComponentType<ListenerComponent>();

  // CameraComponent has constructor overloads, so it supplies a custom add
  // thunk that picks one from the trailing Lua arguments:
  //   node:addComponent(CameraComponent)                       -- default view
  //   node:addComponent(CameraComponent, viewport)             -- FloatRect
  //   node:addComponent(CameraComponent, center, size)         -- two Vector2f
  registerComponentType<CameraComponent>(
    [](sol::state_view lua, SceneNode& node, const sol::variadic_args& args)
      -> sol::object {
      if (args.size() >= 2 &&
          args[0].is<sf::Vector2f>() && args[1].is<sf::Vector2f>()) {
        return sol::make_object(lua,
          node.addComponent<CameraComponent>(args[0].as<sf::Vector2f>(),
                                             args[1].as<sf::Vector2f>()));
      }
      if (args.size() >= 1 && args[0].is<sf::FloatRect>()) {
        return sol::make_object(lua,
          node.addComponent<CameraComponent>(args[0].as<sf::FloatRect>()));
      }
      return sol::make_object(lua, node.addComponent<CameraComponent>());
    });

  registerComponentType<SpriteComponent>();
  registerComponentType<AnimatorComponent>();

  // ParticleSystemComponent can be built bare or from an EmitterConfig, so it
  // supplies a custom add thunk that picks the overload from the Lua arguments:
  //   node:addComponent(ParticleSystemComponent)          -- default config
  //   node:addComponent(ParticleSystemComponent, config)  -- EmitterConfig
  /*
  registerComponentType<ParticleSystemComponent>(
    [](sol::state_view lua, SceneNode& node, const sol::variadic_args& args)
      -> sol::object {
      if (args.size() >= 1 && args[0].is<EmitterConfig>()) {
        return sol::make_object(lua,
          node.addComponent<ParticleSystemComponent>(
            args[0].as<EmitterConfig>()));
      }
      return sol::make_object(lua, node.addComponent<ParticleSystemComponent>());
    });
  */

  // ScriptComponent references its Lua script by asset UUID, so the thunk takes
  // the asset NAME from the trailing Lua argument and resolves it via
  // createFromName (the same id the cooker assigns); without a name there is no
  // valid overload, so it returns nil:
  //   node:addComponent(ScriptComponent, "character.lua")
  registerComponentType<ScriptComponent>(
    [](sol::state_view lua, SceneNode& node, const sol::variadic_args& args)
      -> sol::object {
      if (args.size() >= 1 && args[0].is<std::string>()) {
        return sol::make_object(lua,
          node.addComponent<ScriptComponent>(
            UUID::createFromName(args[0].as<std::string>())));
      }
      return sol::make_object(lua, sol::lua_nil);
    });
}

}  // namespace script

}  // namespace sfmx