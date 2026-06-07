#include "input/ActionMap.h"

namespace sfmx
{

ActionMap::ActionMap(StringView name)
  : m_name(name) {}

InputAction*
ActionMap::addAction(StringView name, ActionValueType::E valueType) {
  m_actions.push_back(MakeUnique<InputAction>(name, valueType));
  return m_actions.back().get();
}

InputAction*
ActionMap::findAction(StringView name) const {
  for (const UniquePtr<InputAction>& action : m_actions) {
    if (action->getName() == name) {
      return action.get();
    }
  }
  return nullptr;
}

void
ActionMap::evaluate(InputSystem& system, float deltaTime) {
  if (!m_enabled) {
    return;
  }
  for (UniquePtr<InputAction>& action : m_actions) {
    if (action->isEnabled()) {
      action->evaluate(system, deltaTime);
    }
  }
}

} // namespace sfmx
