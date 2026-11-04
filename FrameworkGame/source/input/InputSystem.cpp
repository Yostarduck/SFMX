#include "input/InputSystem.h"

#include <SFML/Window/Event.hpp>
#include <SFML/Window/WindowBase.hpp>

#include "input/Gamepad.h"
#include "input/InputControl.h"
#include "input/Keyboard.h"
#include "input/Mouse.h"

namespace sfmx
{

InputSystem::~InputSystem() = default;

void
InputSystem::onStartUp() {
  Keyboard::startUp();
  Mouse::startUp();
  Gamepad::startUp();
}

void
InputSystem::onShutDown() {
  Gamepad::shutDown();
  Mouse::shutDown();
  Keyboard::shutDown();
}

void
InputSystem::beginFrame() {
  Keyboard::instance().beginFrame();
  Mouse::instance().beginFrame();
  Gamepad::instance().beginFrame();
}

void
InputSystem::onEvent(const sf::Event& event) {
  Keyboard::instance().onEvent(event);
  Mouse::instance().onEvent(event);
  Gamepad::instance().onEvent(event);
}

void
InputSystem::update(float deltaTime, const sf::WindowBase& window) {
  Mouse::instance().pollPosition(window);
  Gamepad::instance().poll();

  if (nullptr != m_activeMapping) {
    m_activeMapping->evaluate(*this, deltaTime);
  }
}

float
InputSystem::sampleControl(const InputControl& control) const {
  switch (control.m_device) {
    case DeviceType::kKeyboard:
      return Keyboard::instance().isPressed(static_cast<Key>(control.m_code))
               ? 1.f : 0.f;
    case DeviceType::kMouse:
      return Mouse::instance().isPressed(
               static_cast<MouseButton>(control.m_code)) ? 1.f : 0.f;
    case DeviceType::kGamepad: {
      const int index = control.m_gamepadIndex < 0 ? 0 : control.m_gamepadIndex;
      GamepadDevice& pad = Gamepad::instance().get(index);
      if (control.m_isAxis) {
        return pad.getAxis(static_cast<Axis>(control.m_code));
      }
      return pad.isPressed(static_cast<GamepadButton>(control.m_code))
               ? 1.f : 0.f;
    }
    default:
      return 0.f;
  }
}

Mapping*
InputSystem::createMapping(StringView name) {
  m_mappings.push_back(MakeUnique<Mapping>(name));
  return m_mappings.back().get();
}

bool
InputSystem::destroyMapping(Mapping* mapping) {
  if (nullptr == mapping) {
    return false;
  }
  for (auto it = m_mappings.begin(); it != m_mappings.end(); ++it) {
    if (it->get() == mapping) {
      if (m_activeMapping == mapping) {
        m_activeMapping = nullptr;
      }
      m_mappings.erase(it);
      return true;
    }
  }
  return false;
}

void
InputSystem::clearMappings() {
  m_activeMapping = nullptr;
  m_mappings.clear();
}

} // namespace sfmx
