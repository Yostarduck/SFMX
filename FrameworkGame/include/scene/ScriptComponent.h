#pragma once

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
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

  // Run the script each frame, passing the owning node in as `self`.
  void
  onUpdate(float deltaTime) override;

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

  SPtr<LuaAsset>          m_scriptAsset;                 // keep-alive for the resolved script
  UUID                    m_scriptAssetId = UUID::null();
  sol::protected_function m_script;
  bool                    m_initialized = false;
};

}

DECLARE_TYPE_TRAITS(sfmx::ScriptComponent)
