#pragma once

#include "core/platform/Prerequisites.h"
#include "input/InputAction.h"
#include "input/InputValue.h"

namespace sfmx
{

class InputSystem;

/**
 * @brief A named, toggleable group of @ref InputAction (e.g. "Gameplay", "UI").
 *
 * Owns its actions. Switching control schemes is done by enabling/disabling
 * maps within the active @ref Mapping.
 */
class ActionMap
{
 public:
  explicit ActionMap(StringView name);

  NODISCARD const String&
  getName() const { return m_name; }

  /** @brief Create and register an action; returns a non-owning pointer. */
  InputAction*
  addAction(StringView name, ActionValueType valueType);

  /** @brief Find an action by name, or nullptr. */
  NODISCARD InputAction*
  findAction(StringView name) const;

  void
  enable() { m_enabled = true; }

  void
  disable() { m_enabled = false; }

  NODISCARD bool
  isEnabled() const { return m_enabled; }

  /** @brief (Internal) Evaluate every enabled action in this map. */
  void
  evaluate(InputSystem& system, float deltaTime);

 private:
  String m_name;
  Vector<UniquePtr<InputAction>> m_actions;
  bool m_enabled = true;
};

} // namespace sfmx
