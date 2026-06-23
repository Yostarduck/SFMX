#pragma once

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"

#include <sol/sol.hpp>

namespace sfmx
{

class ScriptEngine;

class ScriptComponent : public ComponentT<ScriptComponent>
{
 public:
  ScriptComponent(SceneNode* owner, std::string_view scriptName);

  /** @brief Deferred ctor: empty script, not initialized. Used by the component
   *         registry / deserializer, which fills the name via @ref onDeserialize
   *         and re-binds through the ScriptEngine. */
  explicit ScriptComponent(SceneNode* owner);

  // Run the script each frame, passing the owning node in as `self`.
  void
  onUpdate(float deltaTime) override;

  /** @brief Serializes the script name (path); the bound function is rebuilt on load. */
  void
  onSerialize(DataStream& stream) const override;
  /** @brief Restores the script name and re-binds via the ScriptEngine when started. */
  void
  onDeserialize(DataStream& stream) override;

  NODISCARD FORCEINLINE bool
  isInitialized() const { return m_initialized; }

  /** @brief The script name/path this component runs (the serialized handle). */
  NODISCARD FORCEINLINE const String&
  getScriptName() const { return m_scriptName; }

 private:
  friend ScriptEngine;

  String m_scriptName;
  sol::protected_function m_script;
  bool m_initialized = false;
};

}

DECLARE_TYPE_TRAITS(sfmx::ScriptComponent)