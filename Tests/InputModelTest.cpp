#include <doctest/doctest.h>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "core/platform/Prerequisites.h"
#include "input/ActionMap.h"
#include "input/InputAction.h"
#include "input/InputControl.h"
#include "input/InputSystem.h"
#include "input/Mapping.h"

using namespace sfmx;

// Input model: validated path parsing plus the removal APIs that scripting
// needs to create, rebind and tear down mappings at runtime.

namespace {

// InputSystem owns the device singletons; start it only if a previous test has
// not already done so, and shut it down only when this scope started it.
struct InputScope
{
  bool started = false;

  InputScope() {
    if (!InputSystem::isStarted()) {
      InputSystem::startUp();
      started = true;
    }
  }

  ~InputScope() {
    if (started && InputSystem::isStarted()) {
      InputSystem::shutDown();
    }
  }
};

} // namespace

TEST_CASE("InputControl::tryFromPath parses canonical control paths") {
  const auto key = InputControl::tryFromPath("Keyboard/Space");
  REQUIRE(key.has_value());
  CHECK(key->m_device == DeviceType::kKeyboard);
  CHECK(key->m_code == static_cast<int>(Key::kSpace));
  CHECK(key->toPath() == "Keyboard/Space");

  const auto button = InputControl::tryFromPath("Mouse/Left");
  REQUIRE(button.has_value());
  CHECK(button->m_device == DeviceType::kMouse);
  CHECK(button->m_code == static_cast<int>(MouseButton::kLeft));
  CHECK(button->toPath() == "Mouse/Left");

  const auto axis = InputControl::tryFromPath("Gamepad2/Axis/LeftX");
  REQUIRE(axis.has_value());
  CHECK(axis->m_device == DeviceType::kGamepad);
  CHECK(axis->m_gamepadIndex == 2);
  CHECK(axis->m_isAxis);
  CHECK(axis->m_code == static_cast<int>(Axis::kLeftX));
  CHECK(axis->toPath() == "Gamepad2/Axis/LeftX");

  const auto padButton = InputControl::tryFromPath("Gamepad0/Button/South");
  REQUIRE(padButton.has_value());
  CHECK(padButton->m_device == DeviceType::kGamepad);
  CHECK(padButton->m_gamepadIndex == 0);
  CHECK_FALSE(padButton->m_isAxis);
  CHECK(padButton->m_code == static_cast<int>(GamepadButton::kSouth));
  CHECK(padButton->toPath() == "Gamepad0/Button/South");
}

TEST_CASE("InputControl::tryFromPath rejects malformed paths") {
  CHECK_FALSE(InputControl::tryFromPath("").has_value());
  CHECK_FALSE(InputControl::tryFromPath("Keyboard").has_value());
  CHECK_FALSE(InputControl::tryFromPath("Keyboard/NotAKey").has_value());
  CHECK_FALSE(InputControl::tryFromPath("Keyboard/Space/Extra").has_value());

  CHECK_FALSE(InputControl::tryFromPath("Mouse/NotAButton").has_value());
  CHECK_FALSE(InputControl::tryFromPath("Mouse/Left/Extra").has_value());

  // Missing index, non-numeric index, out-of-range index.
  CHECK_FALSE(InputControl::tryFromPath("Gamepad/Axis/LeftX").has_value());
  CHECK_FALSE(InputControl::tryFromPath("GamepadX/Axis/LeftX").has_value());
  CHECK_FALSE(InputControl::tryFromPath("Gamepad9/Axis/LeftX").has_value());

  // Missing the Axis/Button segment, or an unknown one.
  CHECK_FALSE(InputControl::tryFromPath("Gamepad0/LeftX").has_value());
  CHECK_FALSE(InputControl::tryFromPath("Gamepad0/Stick/LeftX").has_value());
  CHECK_FALSE(InputControl::tryFromPath("Gamepad0/Axis/NotAnAxis").has_value());
  CHECK_FALSE(InputControl::tryFromPath("Gamepad0/Button/NotAButton").has_value());

  CHECK_FALSE(InputControl::tryFromPath("Joystick0/Axis/LeftX").has_value());
}

TEST_CASE("InputControl::fromPath stays lenient for malformed paths") {
  const InputControl fallback = InputControl::fromPath("not/a/valid/path");
  CHECK(fallback.m_device == DeviceType::kKeyboard);
  CHECK(fallback.m_code == 0);

  const InputControl valid = InputControl::fromPath("Keyboard/Space");
  CHECK(valid.toPath() == "Keyboard/Space");
}

TEST_CASE("InputAction removes and clears bindings, composites and processors") {
  InputScope scope;

  Mapping* mapping = InputSystem::instance().createMapping("TestActionRemoval");
  REQUIRE(mapping != nullptr);
  ActionMap* map = mapping->addMap("Player");
  REQUIRE(map != nullptr);
  InputAction* action = map->addAction("Jump", ActionValueType::kButton);
  REQUIRE(action != nullptr);

  action->addBinding(InputControl::fromPath("Keyboard/Space"));
  action->addBinding(InputControl::fromPath("Gamepad0/Button/South"));
  CHECK(action->getBindingCount() == 2);
  CHECK(action->getBinding(0).m_control.toPath() == "Keyboard/Space");

  CHECK(action->removeBinding(0));
  CHECK(action->getBindingCount() == 1);
  CHECK(action->getBinding(0).m_control.toPath() == "Gamepad0/Button/South");
  CHECK_FALSE(action->removeBinding(5));

  action->clearBindings();
  CHECK(action->getBindingCount() == 0);

  CompositeBinding& composite = action->addComposite(CompositeType::kVector2D);
  composite.m_parts.push_back(
      {InputControl::fromPath("Keyboard/A"), CompositeRole::kNegativeX, {}});
  CHECK(action->getCompositeCount() == 1);
  CHECK(action->getComposite(0).m_parts.size() == 1);
  CHECK(action->removeComposite(0));
  CHECK(action->getCompositeCount() == 0);
  CHECK_FALSE(action->removeComposite(0));

  action->addProcessor(Processor{ProcessorType::kDeadZone, {0.15f, 0.95f}, {}});
  CHECK(action->getProcessorCount() == 1);
  action->clearProcessors();
  CHECK(action->getProcessorCount() == 0);

  CHECK(InputSystem::instance().destroyMapping(mapping));
}

TEST_CASE("ActionMap removes and clears actions") {
  InputScope scope;

  Mapping* mapping = InputSystem::instance().createMapping("TestMapRemoval");
  REQUIRE(mapping != nullptr);
  ActionMap* map = mapping->addMap("Player");
  REQUIRE(map != nullptr);

  map->addAction("Jump", ActionValueType::kButton);
  map->addAction("Move", ActionValueType::kAxis2D);
  CHECK(map->getActionCount() == 2);

  CHECK(map->removeAction("Jump"));
  CHECK_FALSE(map->removeAction("Jump"));
  CHECK(map->getActionCount() == 1);
  CHECK(map->findAction("Jump") == nullptr);
  CHECK(map->findAction("Move") != nullptr);

  map->clearActions();
  CHECK(map->getActionCount() == 0);

  CHECK(InputSystem::instance().destroyMapping(mapping));
}

TEST_CASE("Mapping removes and clears action maps") {
  InputScope scope;

  Mapping* mapping = InputSystem::instance().createMapping("TestMappingRemoval");
  REQUIRE(mapping != nullptr);

  mapping->addMap("UI");
  mapping->addMap("Player");
  CHECK(mapping->getMapCount() == 2);

  CHECK(mapping->removeMap("UI"));
  CHECK_FALSE(mapping->removeMap("UI"));
  CHECK(mapping->getMapCount() == 1);
  CHECK(mapping->findMap("UI") == nullptr);

  mapping->clearMaps();
  CHECK(mapping->getMapCount() == 0);

  CHECK(InputSystem::instance().destroyMapping(mapping));
}

TEST_CASE("InputSystem destroys mappings and clears the active pointer") {
  InputScope scope;

  InputSystem& system = InputSystem::instance();
  Mapping* mapping = system.createMapping("TestActiveRemoval");
  REQUIRE(mapping != nullptr);
  system.setActiveMapping(mapping);
  CHECK(system.getActiveMapping() == mapping);

  CHECK(system.destroyMapping(mapping));
  CHECK(system.getActiveMapping() == nullptr);
  CHECK_FALSE(system.destroyMapping(mapping));
  CHECK_FALSE(system.destroyMapping(nullptr));
}

TEST_CASE("InputSystem::clearMappings destroys every mapping") {
  InputScope scope;

  InputSystem& system = InputSystem::instance();
  system.createMapping("First");
  Mapping* second = system.createMapping("Second");
  REQUIRE(second != nullptr);
  system.setActiveMapping(second);
  CHECK(system.getMappingCount() == 2);

  system.clearMappings();
  CHECK(system.getMappingCount() == 0);
  CHECK(system.getActiveMapping() == nullptr);
}

TEST_CASE("A removed binding stops contributing to the action value") {
  InputScope scope;

  InputSystem& system = InputSystem::instance();
  Mapping* mapping = system.createMapping("TestBindingEvaluation");
  REQUIRE(mapping != nullptr);
  ActionMap* map = mapping->addMap("Player");
  REQUIRE(map != nullptr);
  InputAction* jump = map->addAction("Jump", ActionValueType::kButton);
  REQUIRE(jump != nullptr);
  jump->addBinding(InputControl::fromPath("Keyboard/Space"));
  system.setActiveMapping(mapping);

  system.onEvent(sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Space}});
  mapping->evaluate(system, 0.016f);
  CHECK(jump->getValue().asBool());

  jump->clearBindings();
  mapping->evaluate(system, 0.016f);
  CHECK_FALSE(jump->getValue().asBool());

  CHECK(system.destroyMapping(mapping));
}
