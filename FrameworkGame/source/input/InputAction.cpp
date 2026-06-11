#include "input/InputAction.h"

#include <algorithm>
#include <cmath>

namespace sfmx
{

InputAction::InputAction(StringView name, ActionValueType::E valueType)
  : m_name(name),
    m_valueType(valueType) {
  m_value.m_type = valueType;
}

Binding&
InputAction::addBinding(const InputControl& control) {
  m_bindings.push_back(Binding{control, {}});
  return m_bindings.back();
}

CompositeBinding&
InputAction::addComposite(CompositeType::E type) {
  m_composites.push_back(CompositeBinding{type, {}});
  return m_composites.back();
}

void
InputAction::addProcessor(const Processor& processor) {
  m_processors.push_back(processor);
}

InputValue
InputAction::computeValue(InputSystem& system) const {
  Vector2f acc{0.f, 0.f};

  for (const Binding& binding : m_bindings) {
    const float raw = binding.m_control.read(system);
    const Vector2f value =
      applyProcessors(binding.m_processors, Vector2f{raw, 0.f}, m_valueType);
    if (ActionValueType::kButton == m_valueType) {
      acc.x = std::max(acc.x, value.x);  // any binding triggers (logical OR)
    }
    else if (std::fabs(value.x) > std::fabs(acc.x)) {
      acc.x = value.x;  // axis: strongest binding wins
    }
  }

  for (const CompositeBinding& composite : m_composites) {
    const InputValue value = composite.evaluate(system);
    if (std::fabs(value.m_value.x) > std::fabs(acc.x)) {
      acc.x = value.m_value.x;
    }
    if (std::fabs(value.m_value.y) > std::fabs(acc.y)) {
      acc.y = value.m_value.y;
    }
  }

  acc = applyProcessors(m_processors, acc, m_valueType);

  InputValue out;
  out.m_type = m_valueType;
  out.m_value = acc;
  return out;
}

void
InputAction::evaluate(InputSystem& system, float deltaTime) {
  if (!m_enabled) {
    m_phase = ActionPhase::kDisabled;
    return;
  }

  m_value = computeValue(system);
  const bool actuated = m_value.magnitude() > 0.f;

  const ActionPhase::E previous = m_phase;
  const ActionPhase::E current = m_interaction.step(actuated, deltaTime);
  m_phase = current;

  const bool wasActuated =
    ActionPhase::kStarted == previous || ActionPhase::kPerformed == previous;
  const bool isActuated =
    ActionPhase::kStarted == current || ActionPhase::kPerformed == current;

  InputContext context;
  context.m_action = this;
  context.m_value = m_value;
  context.m_deltaTime = deltaTime;

  // Started: rising edge into an actuated phase.
  if (isActuated && !wasActuated) {
    context.m_phase = ActionPhase::kStarted;
    m_onStarted(context);
  }

  // Performed: fired this frame per the interaction (every non-zero frame for
  // the default pass-through).
  if (ActionPhase::kPerformed == current) {
    context.m_phase = ActionPhase::kPerformed;
    m_onPerformed(context);
  }

  // Canceled: an actuated/holding action ended or its interaction aborted.
  if (ActionPhase::kCanceled == current &&
      ActionPhase::kWaiting != previous &&
      ActionPhase::kCanceled != previous &&
      ActionPhase::kDisabled != previous) {
    context.m_phase = ActionPhase::kCanceled;
    m_onCanceled(context);
  }
}

} // namespace sfmx
