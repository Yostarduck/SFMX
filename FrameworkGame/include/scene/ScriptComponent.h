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
   *         and re-binds through the ScriptEngine.
   */
  explicit ScriptComponent(SceneNode* owner);

  /** @brief Fires the script's optional `onDestroyed(self)` hook
   *         before the component is torn down.
   */
  ~ScriptComponent() override;

  /** @brief Records that the component is linked and fires `onCreated` once the
   *         script is also bound.
   */
  void
  onAttached() override;

  // Run the script each frame, passing the owning node in as `self`. Fires the
  // one-shot `onStart(self)` hook just before the first `onUpdate`.
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

  /** @brief Fires `onCreated(self)` exactly once, but only after the script is
   *         both bound and linked (the bind and link can complete in either
   *         order). Called from both @ref onAttached and the bind sites.
   */
  void
  triggerOnCreated();

  /** @brief Invokes @p fn as `fn(self)`, logging any error. No-op if @p fn
   *         is not a valid function (the hook was omitted by the script).
   */
  void
  callHook(const sol::protected_function& fn);

  String m_scriptName;

  sol::protected_function m_onCreated;
  sol::protected_function m_onStart;
  sol::protected_function m_onUpdate;
  sol::protected_function m_onDestroyed;

  bool m_initialized = false;  // script compiled and hooks bound
  bool m_linked      = false;  // onAttached has run
  bool m_created     = false;  // onCreated already fired (one-shot)
  bool m_started     = false;  // onStart already fired (one-shot)
};

}

DECLARE_TYPE_TRAITS(sfmx::ScriptComponent)