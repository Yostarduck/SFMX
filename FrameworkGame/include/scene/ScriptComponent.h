#pragma once

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "utils/EventSystem.h"
#include "utils/UUID.h"

#include <sol/sol.hpp>

namespace sfmx
{

class ScriptEngine;
class LuaAsset;

class ScriptComponent : public ComponentT<ScriptComponent>
{
 public:
  /** @brief Attach and bind the script referenced by @p scriptAssetId (a @ref LuaAsset). */
  ScriptComponent(SceneNode* owner, const UUID& scriptAssetId);

  /** @brief Deferred ctor: no script yet, not initialized. Used by the component
   *         registry / deserializer, which sets the asset id via @ref onDeserialize
   *         and re-binds through the ScriptEngine. */
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

  /** @brief Run the script each frame, passing the owning node in as `self`. Fires the
   *         one-shot `onStart(self)` hook just before the first `onUpdate`.
   */
  void
  onUpdate(float deltaTime) override;

  /** @brief Execute a function exported by the script, passing
   *         the owning node in as `self`.
   */
  template<typename... Args>
  void
  executeFunction(const String& fnName, Args&&... args) const {
    if (!m_initialized) {
      return;
    }

    const auto it = m_exportedFunctions.find(UUID::createFromName(fnName));
    if (it == m_exportedFunctions.end()) {
      return;
    }

    const sol::protected_function& fn = it->second;
    if (!fn.valid()) {
      return;
    }

    const sol::protected_function_result result =
      fn(getOwner(), std::forward<Args>(args)...);
    if (!result.valid()) {
      const sol::error err = result;
    }
  }

  /** @brief Register an event handle to keep it alive until the component is destroyed. */
  void
  registerEvent(HEvent&& event);

  /** @brief Unregister all events, causing them to be destroyed. */
  FORCEINLINE void unregisterAllEvents() { m_events.clear(); }

  /** @brief Bind to a @ref LuaAsset, keeping it alive and recording its UUID; the
   *         script re-binds through the ScriptEngine when both are running. */
  void
  setScriptAsset(SPtr<LuaAsset> asset);

  /** @brief Record the script asset UUID and resolve it via AssetManager when
   *         running; otherwise keep the id so it re-serializes and resolves later. */
  void
  setScriptAssetId(const UUID& id);

  /** @brief UUID of the referenced @ref LuaAsset (the serialized handle). */
  NODISCARD FORCEINLINE const UUID&
  getScriptAssetId() const { return m_scriptAssetId; }

  /** @brief The kept-alive @ref LuaAsset, or nullptr if not resolved. */
  NODISCARD FORCEINLINE SPtr<LuaAsset>
  getScriptAsset() const { return m_scriptAsset; }

  /** @brief Serializes the script asset UUID; the bound function is rebuilt on load. */
  void
  onSerialize(DataStream& stream) const override;
  /** @brief Restores the UUID and re-binds via the ScriptEngine when started. */
  void
  onDeserialize(DataStream& stream) override;

  NODISCARD FORCEINLINE bool
  isInitialized() const { return m_initialized; }

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

  SPtr<LuaAsset>          m_scriptAsset;                 // keep-alive for the resolved script
  UUID                    m_scriptAssetId = UUID::null();
  
  sol::protected_function m_onCreated;
  sol::protected_function m_onStart;
  sol::protected_function m_onUpdate;
  sol::protected_function m_onDestroyed;

  bool                    m_initialized = false;  // script compiled and hooks bound
  bool                    m_linked      = false;  // onAttached has run
  bool                    m_created     = false;  // onCreated already fired (one-shot)
  bool                    m_started     = false;  // onStart already fired (one-shot)

  UnorderedMap<UUID, sol::protected_function> m_exportedFunctions;

  Vector<HEvent> m_events;
};

}

DECLARE_TYPE_TRAITS(sfmx::ScriptComponent)
