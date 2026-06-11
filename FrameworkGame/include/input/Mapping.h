#pragma once

#include "core/platform/Prerequisites.h"
#include "input/ActionMap.h"

namespace sfmx
{

class InputSystem;

/**
 * @brief The root of the mapping data model: a named list of @ref ActionMap.
 *
 * Owns its maps. Exactly one Mapping is active in the @ref InputSystem at a
 * time.
 */
class Mapping
{
 public:
  explicit Mapping(StringView name);

  NODISCARD const String&
  getName() const { return m_name; }

  /** @brief Create and register an action map; returns a non-owning pointer. */
  ActionMap*
  addMap(StringView name);

  /** @brief Find an action map by name, or nullptr. */
  NODISCARD ActionMap*
  findMap(StringView name) const;

  /** @brief (Internal) Evaluate every enabled map in this mapping. */
  void
  evaluate(InputSystem& system, float deltaTime);

 private:
  String m_name;
  Vector<UniquePtr<ActionMap>> m_maps;
};

} // namespace sfmx
