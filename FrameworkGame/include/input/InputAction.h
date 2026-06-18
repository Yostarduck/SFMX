#pragma once

#include "core/platform/Prerequisites.h"
#include "input/InputControl.h"
#include "input/Interaction.h"
#include "input/InputValue.h"
#include "input/Processor.h"
#include "utils/EventSystem.h"

namespace sfmx
{

class InputAction;
class InputSystem;

/** @brief Payload passed to every action callback. */
struct InputContext
{
  const InputAction* m_action = nullptr;
  InputValue m_value;
  ActionPhase m_phase = ActionPhase::kWaiting;
  float m_deltaTime = 0.f;
};

/**
 * @brief A named action: a typed value, a set of bindings (simple and/or
 *        composite), a processor chain, an interaction, and three events.
 *
 * Each frame @ref evaluate samples the bindings, runs the value through the
 * processor chain and the interaction state machine, and raises
 * started / performed / canceled as transitions occur.
 */
class InputAction
{
 public:
  InputAction(StringView name, ActionValueType valueType);

  NODISCARD const String&
  getName() const { return m_name; }

  NODISCARD ActionValueType
  getValueType() const { return m_valueType; }

  NODISCARD const InputValue&
  getValue() const { return m_value; }

  NODISCARD ActionPhase
  getPhase() const { return m_phase; }

  /** @brief Add a simple binding and return it for further configuration. */
  Binding&
  addBinding(const InputControl& control);

  /** @brief Add a composite binding and return it for further configuration. */
  CompositeBinding&
  addComposite(CompositeType type);

  /** @brief Append an action-level processor (applied after combination). */
  void
  addProcessor(const Processor& processor);

  /** @brief Set the interaction (timing model). */
  void
  setInteraction(const Interaction& interaction) { m_interaction = interaction; }

  /** @brief Subscribe to the started phase; returns an RAII unsubscribe handle. */
  NODISCARD HEvent
  onStarted(Function<void(const InputContext&)> callback) const {
    return m_onStarted.connect(callback);
  }

  NODISCARD HEvent
  onPerformed(Function<void(const InputContext&)> callback) const {
    return m_onPerformed.connect(callback);
  }

  NODISCARD HEvent
  onCanceled(Function<void(const InputContext&)> callback) const {
    return m_onCanceled.connect(callback);
  }

  void
  enable() { m_enabled = true; }

  void
  disable() { m_enabled = false; }

  NODISCARD bool
  isEnabled() const { return m_enabled; }

  /** @brief (Internal) Sample bindings, run the phase machine, fire events. */
  void
  evaluate(InputSystem& system, float deltaTime);

 private:
  NODISCARD InputValue
  computeValue(InputSystem& system) const;

  String m_name;
  ActionValueType m_valueType;
  Vector<Binding> m_bindings;
  Vector<CompositeBinding> m_composites;
  Vector<Processor> m_processors;
  Interaction m_interaction;
  bool m_enabled = true;
  ActionPhase m_phase = ActionPhase::kWaiting;
  InputValue m_value;

  Event<void(const InputContext&)> m_onStarted;
  Event<void(const InputContext&)> m_onPerformed;
  Event<void(const InputContext&)> m_onCanceled;
};

} // namespace sfmx
