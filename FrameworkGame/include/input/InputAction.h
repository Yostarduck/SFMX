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

  NODISCARD FORCEINLINE const String&
  getName() const { return m_name; }

  NODISCARD FORCEINLINE ActionValueType
  getValueType() const { return m_valueType; }

  NODISCARD FORCEINLINE const InputValue&
  getValue() const { return m_value; }

  NODISCARD FORCEINLINE ActionPhase
  getPhase() const { return m_phase; }

  /** @brief Add a simple binding and return it for further configuration. */
  Binding&
  addBinding(const InputControl& control);

  /** @brief Add a composite binding and return it for further configuration. */
  CompositeBinding&
  addComposite(CompositeType type);

  /** @brief Number of simple bindings. */
  NODISCARD size_t
  getBindingCount() const { return m_bindings.size(); }

  /** @brief The simple binding at @p index (valid while < getBindingCount()). */
  NODISCARD const Binding&
  getBinding(size_t index) const { return m_bindings[index]; }

  /** @brief Remove the simple binding at @p index; false if out of range. */
  bool
  removeBinding(size_t index);

  /** @brief Remove every simple binding (composites are untouched). */
  void
  clearBindings() { m_bindings.clear(); }

  /** @brief Number of composite bindings. */
  NODISCARD size_t
  getCompositeCount() const { return m_composites.size(); }

  /** @brief The composite binding at @p index (valid while < getCompositeCount()). */
  NODISCARD const CompositeBinding&
  getComposite(size_t index) const { return m_composites[index]; }

  /** @brief Remove the composite binding at @p index; false if out of range. */
  bool
  removeComposite(size_t index);

  /** @brief Remove every composite binding (simple bindings are untouched). */
  void
  clearComposites() { m_composites.clear(); }

  /** @brief Append an action-level processor (applied after combination). */
  void
  addProcessor(const Processor& processor);

  /** @brief Number of action-level processors. */
  NODISCARD size_t
  getProcessorCount() const { return m_processors.size(); }

  /** @brief Remove every action-level processor. */
  void
  clearProcessors() { m_processors.clear(); }

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

  NODISCARD FORCEINLINE bool
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
