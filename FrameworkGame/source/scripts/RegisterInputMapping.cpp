#include "scripts/RegisterInputMapping.h"

#include <utility>

#include "core/platform/Prerequisites.h"
#include "input/InputControl.h"
#include "input/InputValue.h"
#include "input/InputAction.h"
#include "input/ActionMap.h"
#include "input/Mapping.h"
#include "input/InputSystem.h"
#include "input/Interaction.h"
#include "input/Processor.h"
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

// Remove the first simple binding whose control renders to @p path. Scripts
// bind/unbind by the same human-readable path, so they never handle indices.
bool removeBindingByPath(InputAction& action, StringView path) {
  for (size_t i = 0; i < action.getBindingCount(); ++i) {
    if (action.getBinding(i).m_control.toPath() == path) {
      return action.removeBinding(i);
    }
  }
  return false;
}

}  // namespace

void
registerInputMapping(sol::state_view lua) {
  lua.new_usertype<InputControl>("InputControl",
    sol::no_constructor,

    "toPath", &InputControl::toPath,

    "device",       sol::readonly(&InputControl::m_device),
    "code",         sol::readonly(&InputControl::m_code),
    "gamepadIndex", sol::readonly(&InputControl::m_gamepadIndex),
    "isAxis",       sol::readonly(&InputControl::m_isAxis));

  // Validated parser: nil on a malformed path, so scripts can report a typo
  // instead of silently binding the default keyboard control.
  sol::table inputControl = lua["InputControl"];
  inputControl["fromPath"] =
    [](sol::this_state state, const String& path) -> sol::object {
      const Optional<InputControl> control = InputControl::tryFromPath(path);
      if (!control.has_value()) {
        return sol::make_object(state, sol::lua_nil);
      }
      return sol::make_object(state, *control);
    };

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

    // Bind one control. Strings use InputControl's path grammar and are
    // validated; a bad path returns false and adds nothing.
    "bind", sol::overload(
      [](InputAction& action, const InputControl& control) {
        action.addBinding(control);
        return true;
      },
      [](InputAction& action, const String& path) {
        const Optional<InputControl> control = InputControl::tryFromPath(path);
        if (!control.has_value()) {
          return false;
        }
        action.addBinding(*control);
        return true;
      }),

    // Build a composite from a list of { path = "...", role = CompositeRole.X }
    // parts. Everything is validated before the composite is attached, so a
    // bad part leaves the action unchanged.
    "bindComposite", [](InputAction& action, sol::table parts,
                        sol::optional<CompositeType> type) {
      Vector<BindingPart> validated;
      for (const auto& entry : parts) {
        if (!entry.second.is<sol::table>()) {
          return false;
        }
        sol::table part = entry.second.as<sol::table>();

        const sol::optional<String> path =
          part.get<sol::optional<String>>("path");
        const sol::optional<CompositeRole> role =
          part.get<sol::optional<CompositeRole>>("role");
        if (!path.has_value() || !role.has_value()) {
          return false;
        }

        const Optional<InputControl> control =
          InputControl::tryFromPath(*path);
        if (!control.has_value()) {
          return false;
        }

        BindingPart bindingPart;
        bindingPart.m_control = *control;
        bindingPart.m_role = *role;
        validated.push_back(bindingPart);
      }

      if (validated.empty()) {
        return false;
      }

      CompositeBinding& composite =
        action.addComposite(type.value_or(CompositeType::kVector2D));
      composite.m_parts = std::move(validated);
      return true;
    },

    // Remove the first binding matching a path (or a control's path).
    "unbind", sol::overload(
      [](InputAction& action, const InputControl& control) {
        return removeBindingByPath(action, control.toPath());
      },
      [](InputAction& action, const String& path) {
        return removeBindingByPath(action, path);
      }),

    // 1-based index access, matching Lua conventions; prefer unbind(path).
    "removeBinding", [](InputAction& action, int index) {
      if (index < 1) {
        return false;
      }
      return action.removeBinding(static_cast<size_t>(index - 1));
    },
    "getBindingCount", &InputAction::getBindingCount,
    "getBindingPath", [](const InputAction& action, int index) -> String {
      if (index < 1 ||
          static_cast<size_t>(index) > action.getBindingCount()) {
        return String();
      }
      return action.getBinding(static_cast<size_t>(index - 1)).m_control.toPath();
    },
    "clearBindings", &InputAction::clearBindings,

    "getCompositeCount", &InputAction::getCompositeCount,

    // 1-based composite index, matching removeBinding.
    "removeComposite", [](InputAction& action, int index) {
      if (index < 1) {
        return false;
      }
      return action.removeComposite(static_cast<size_t>(index - 1));
    },
    "clearComposites", &InputAction::clearComposites,

    // Composite introspection by 1-based index: type, part count, and each
    // part's control path and role. Enough for a rebinding UI without exposing
    // the BindingPart/Processor internals.
    "getCompositeType", [](const InputAction& action, int index,
                           sol::this_state state) -> sol::object {
      if (index < 1 ||
          static_cast<size_t>(index) > action.getCompositeCount()) {
        return sol::make_object(state, sol::lua_nil);
      }
      return sol::make_object(
        state, action.getComposite(static_cast<size_t>(index - 1)).m_type);
    },
    "getCompositePartCount", [](const InputAction& action, int index) -> size_t {
      if (index < 1 ||
          static_cast<size_t>(index) > action.getCompositeCount()) {
        return 0;
      }
      return action.getComposite(static_cast<size_t>(index - 1)).m_parts.size();
    },
    "getCompositePartPath", [](const InputAction& action, int compositeIndex,
                               int partIndex) -> String {
      if (compositeIndex < 1 ||
          static_cast<size_t>(compositeIndex) > action.getCompositeCount()) {
        return String();
      }
      const CompositeBinding& composite =
        action.getComposite(static_cast<size_t>(compositeIndex - 1));
      if (partIndex < 1 ||
          static_cast<size_t>(partIndex) > composite.m_parts.size()) {
        return String();
      }
      return composite.m_parts[static_cast<size_t>(partIndex - 1)]
        .m_control.toPath();
    },
    "getCompositePartRole", [](const InputAction& action, int compositeIndex,
                               int partIndex,
                               sol::this_state state) -> sol::object {
      if (compositeIndex < 1 ||
          static_cast<size_t>(compositeIndex) > action.getCompositeCount()) {
        return sol::make_object(state, sol::lua_nil);
      }
      const CompositeBinding& composite =
        action.getComposite(static_cast<size_t>(compositeIndex - 1));
      if (partIndex < 1 ||
          static_cast<size_t>(partIndex) > composite.m_parts.size()) {
        return sol::make_object(state, sol::lua_nil);
      }
      return sol::make_object(
        state,
        composite.m_parts[static_cast<size_t>(partIndex - 1)].m_role);
    },

    // Timing model: setInteraction(InteractionType.Hold, 0.4).
    "setInteraction", [](InputAction& action, InteractionType type,
                         sol::optional<float> duration,
                         sol::optional<float> tapSpacing,
                         sol::optional<int> tapCount) {
      Interaction interaction;
      interaction.m_type = type;
      if (duration.has_value()) {
        interaction.m_duration = *duration;
      }
      if (tapSpacing.has_value()) {
        interaction.m_tapSpacing = *tapSpacing;
      }
      if (tapCount.has_value()) {
        interaction.m_tapCount = *tapCount;
      }
      action.setInteraction(interaction);
    },
    "getInteraction", [](const InputAction& action, sol::this_state state) {
      const Interaction& interaction = action.getInteraction();
      sol::table result = sol::state_view(state).create_table();
      result["type"] = interaction.m_type;
      result["duration"] = interaction.m_duration;
      result["tapSpacing"] = interaction.m_tapSpacing;
      result["tapCount"] = interaction.m_tapCount;
      return result;
    },

    // Value transform: addProcessor(ProcessorType.DeadZone, 0.15, 0.95).
    "addProcessor", [](InputAction& action, ProcessorType type,
                       sol::optional<float> ax, sol::optional<float> ay,
                       sol::optional<float> bx, sol::optional<float> by) {
      Processor processor;
      processor.m_type = type;
      if (ax.has_value()) {
        processor.m_paramA.x = *ax;
      }
      if (ay.has_value()) {
        processor.m_paramA.y = *ay;
      }
      if (bx.has_value()) {
        processor.m_paramB.x = *bx;
      }
      if (by.has_value()) {
        processor.m_paramB.y = *by;
      }
      action.addProcessor(processor);
    },
    "getProcessorCount", &InputAction::getProcessorCount,
    "clearProcessors",   &InputAction::clearProcessors,

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
    "addAction", [](ActionMap& map, const String& name,
                    ActionValueType type) {
      return map.addAction(name, type);
    },
    "removeAction", [](ActionMap& map, const String& name) {
      return map.removeAction(name);
    },
    "clearActions",   &ActionMap::clearActions,
    "getActionCount", &ActionMap::getActionCount,
    "enable",         &ActionMap::enable,
    "disable",        &ActionMap::disable,
    "isEnabled",      &ActionMap::isEnabled);

  lua.new_usertype<Mapping>("Mapping",
    sol::no_constructor,

    "getName", &Mapping::getName,
    "findMap", &Mapping::findMap,
    "addMap", [](Mapping& mapping, const String& name) {
      return mapping.addMap(name);
    },
    "removeMap", [](Mapping& mapping, const String& name) {
      return mapping.removeMap(name);
    },
    "clearMaps",   &Mapping::clearMaps,
    "getMapCount", &Mapping::getMapCount);

  lua.new_usertype<InputSystem>("InputSystem",
    sol::no_constructor,

    "createMapping", [](InputSystem& system, const String& name) {
      return system.createMapping(name);
    },
    "destroyMapping",   &InputSystem::destroyMapping,
    "clearMappings",    &InputSystem::clearMappings,
    "getMappingCount",  &InputSystem::getMappingCount,
    "setActiveMapping", &InputSystem::setActiveMapping,
    "getActiveMapping", &InputSystem::getActiveMapping,
    "sampleControl",    &InputSystem::sampleControl);

  lua["InputSystem"] = std::ref(InputSystem::instance());
}

}  // namespace script

}  // namespace sfmx
