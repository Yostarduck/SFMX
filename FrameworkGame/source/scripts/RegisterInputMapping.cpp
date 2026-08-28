#include "scripts/RegisterInputMapping.h"

#include <utility>

#include "core/platform/Prerequisites.h"
#include "input/InputValue.h"
#include "input/InputAction.h"
#include "input/ActionMap.h"
#include "input/Mapping.h"
#include "input/InputSystem.h"
#include "scene/ScriptComponent.h"

namespace sfmx
{

namespace script
{

namespace
{

// Mirrors scriptEventBinder in RegisterUIWidget.cpp, but for the three action
// events: it subscribes a script's exported function to the action and hands the
// resulting handle to the ScriptComponent under a token, so the script can drop
// this one subscription later with unbind(token).
template<typename Connect>
auto actionEventBinder(Connect connect) {
  return [connect](const InputAction& action,
                   ScriptComponent* target,
                   const String& fnName) -> uint32 {
    if (nullptr == target) {
      return 0u;
    }
    HEvent handle = (action.*connect)(
      [target, fnName](const InputContext& context) {
        target->executeFunction(fnName, context);
      });
    return target->registerEventKeyed(std::move(handle));
  };
}

}  // namespace

void
registerInputMapping(sol::state_view lua) {
  lua.new_usertype<InputValue>("InputValue",
    sol::no_constructor,

    "type",      sol::readonly(&InputValue::m_type),
    "asBool",    &InputValue::asBool,
    "asFloat",   &InputValue::asFloat,
    "asVector2", &InputValue::asVector2,
    "magnitude", &InputValue::magnitude);

  lua.new_usertype<InputAction>("InputAction",
    sol::no_constructor,

    "getName",      &InputAction::getName,
    "getValueType", &InputAction::getValueType,
    "getValue",     &InputAction::getValue,
    "getPhase",     &InputAction::getPhase,
    "enable",       &InputAction::enable,
    "disable",      &InputAction::disable,
    "isEnabled",    &InputAction::isEnabled,

    "onStarted",    actionEventBinder(&InputAction::onStarted),
    "onPerformed",  actionEventBinder(&InputAction::onPerformed),
    "onCanceled",   actionEventBinder(&InputAction::onCanceled));

  lua.new_usertype<InputContext>("InputContext",
    sol::no_constructor,

    "action",    sol::readonly(&InputContext::m_action),
    "value",     sol::readonly(&InputContext::m_value),
    "phase",     sol::readonly(&InputContext::m_phase),
    "deltaTime", sol::readonly(&InputContext::m_deltaTime));

  lua.new_usertype<ActionMap>("ActionMap",
    sol::no_constructor,

    "getName",    &ActionMap::getName,
    "findAction", &ActionMap::findAction,
    "enable",     &ActionMap::enable,
    "disable",    &ActionMap::disable,
    "isEnabled",  &ActionMap::isEnabled);

  lua.new_usertype<Mapping>("Mapping",
    sol::no_constructor,

    "getName", &Mapping::getName,
    "findMap", &Mapping::findMap);

  lua.new_usertype<InputSystem>("InputSystem",
    sol::no_constructor,

    "getActiveMapping", &InputSystem::getActiveMapping);

  lua["InputSystem"] = std::ref(InputSystem::instance());
}

}  // namespace script

}  // namespace sfmx
