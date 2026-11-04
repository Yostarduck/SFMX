#include <doctest/doctest.h>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "core/platform/Prerequisites.h"
#include "input/ActionMap.h"
#include "input/InputAction.h"
#include "input/InputControl.h"
#include "input/InputSystem.h"
#include "input/Mapping.h"
#include "scripts/RegisterAll.h"

using namespace sfmx;

// Scripting surface for the input system: a Lua script can create mappings,
// maps and actions, bind/unbind controls by path, configure interactions and
// processors, and tear the mapping down again.

namespace {

struct InputScriptScope
{
  bool started = false;

  InputScriptScope() {
    if (!InputSystem::isStarted()) {
      InputSystem::startUp();
      started = true;
    }
  }

  ~InputScriptScope() {
    if (InputSystem::isStarted()) {
      InputSystem::instance().clearMappings();
    }
    if (started && InputSystem::isStarted()) {
      InputSystem::shutDown();
    }
  }
};

// Run a chunk and surface a Lua error as a doctest failure.
void
runLua(sol::state& lua, const char* code) {
  sol::protected_function_result result =
      lua.safe_script(code, sol::script_pass_on_error);
  if (!result.valid()) {
    const sol::error err = result;
    FAIL_CHECK(err.what());
  }
  REQUIRE(result.valid());
}

} // namespace

TEST_CASE("Lua creates a mapping and binds controls by path") {
  InputScriptScope scope;

  sol::state lua;
  script::registerAll(lua);

  runLua(lua, R"(
    local mapping = InputSystem:createMapping("LuaControls")
    local player = mapping:addMap("Player")
    local jump = player:addAction("Jump", ActionValueType.Button)

    assert(jump:bind("Keyboard/Space"))
    assert(jump:bind("Gamepad0/Button/South"))
    assert(jump:getBindingCount() == 2)
    assert(jump:getBindingPath(1) == "Keyboard/Space")
    assert(jump:getBindingPath(2) == "Gamepad0/Button/South")

    InputSystem:setActiveMapping(mapping)
  )");

  InputSystem& system = InputSystem::instance();
  Mapping* mapping = system.getActiveMapping();
  REQUIRE(mapping != nullptr);
  CHECK(mapping->getName() == "LuaControls");

  ActionMap* player = mapping->findMap("Player");
  REQUIRE(player != nullptr);
  InputAction* jump = player->findAction("Jump");
  REQUIRE(jump != nullptr);
  CHECK(jump->getBindingCount() == 2);
  CHECK(jump->getBinding(0).m_control.toPath() == "Keyboard/Space");
  CHECK(jump->getValueType() == ActionValueType::kButton);

  // The Lua-authored binding actually drives evaluation.
  system.onEvent(sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Space}});
  mapping->evaluate(system, 0.016f);
  CHECK(jump->getValue().asBool());
}

TEST_CASE("Lua validates paths and can unbind controls") {
  InputScriptScope scope;

  sol::state lua;
  script::registerAll(lua);

  runLua(lua, R"(
    local mapping = InputSystem:createMapping("LuaRebind")
    local map = mapping:addMap("Player")
    local jump = map:addAction("Jump", ActionValueType.Button)

    assert(jump:bind("Keyboard/Space"))
    assert(jump:bind("Keyboard/NotAKey") == false)
    assert(jump:getBindingCount() == 1)

    assert(jump:unbind("Keyboard/Space"))
    assert(jump:unbind("Keyboard/Space") == false)
    assert(jump:getBindingCount() == 0)

    local mouse = InputControl.fromPath("Mouse/Left")
    assert(mouse ~= nil)
    assert(jump:bind(mouse))
    assert(jump:getBindingPath(1) == "Mouse/Left")
    assert(jump:unbind(mouse))
    assert(jump:getBindingCount() == 0)

    InputSystem:setActiveMapping(mapping)
    assert(InputSystem:destroyMapping(mapping))
    assert(InputSystem:getActiveMapping() == nil)
  )");
}

TEST_CASE("Lua builds composite bindings and configures interactions") {
  InputScriptScope scope;

  sol::state lua;
  script::registerAll(lua);

  runLua(lua, R"(
    local mapping = InputSystem:createMapping("LuaComposite")
    local player = mapping:addMap("Player")
    local move = player:addAction("Move", ActionValueType.Axis2D)

    assert(move:bindComposite({
      { path = "Keyboard/A",          role = CompositeRole.NegativeX },
      { path = "Keyboard/D",          role = CompositeRole.PositiveX },
      { path = "Gamepad0/Axis/LeftX", role = CompositeRole.AxisX },
    }))
    assert(move:getCompositeCount() == 1)

    -- A bad part rejects the whole composite and leaves the existing one alone.
    assert(move:bindComposite({
      { path = "Keyboard/NotAKey", role = CompositeRole.PositiveX },
    }) == false)
    assert(move:getCompositeCount() == 1)

    move:setInteraction(InteractionType.Hold, 0.4)
    move:addProcessor(ProcessorType.DeadZone, 0.15, 0.95)
    assert(move:getProcessorCount() == 1)
    move:clearProcessors()
    assert(move:getProcessorCount() == 0)

    InputSystem:destroyMapping(mapping)
  )");
}

TEST_CASE("Lua parses InputControl paths and polls a control") {
  InputScriptScope scope;

  sol::state lua;
  script::registerAll(lua);

  runLua(lua, R"(
    local key = InputControl.fromPath("Keyboard/Space")
    assert(key ~= nil)
    assert(key:toPath() == "Keyboard/Space")
    assert(key.device == DeviceType.Keyboard)
    assert(key.code == Key.Space)
    assert(key.isAxis == false)

    local axis = InputControl.fromPath("Gamepad1/Axis/LeftX")
    assert(axis ~= nil)
    assert(axis.gamepadIndex == 1)
    assert(axis.isAxis == true)
    assert(axis:toPath() == "Gamepad1/Axis/LeftX")

    assert(InputControl.fromPath("nonsense") == nil)
    assert(InputControl.fromPath("Gamepad9/Axis/LeftX") == nil)
  )");

  // Polling reads the same device state the actions sample.
  InputSystem::instance().onEvent(
      sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Space}});
  lua["space"] = InputControl::fromPath("Keyboard/Space");
  runLua(lua, "assert(InputSystem:sampleControl(space) == 1.0)");
}
