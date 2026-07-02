#include "scene/ScriptComponent.h"
#include "scene/SceneNode.h"

#include "assets/AssetManager.h"
#include "assets/LuaAsset.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"   // operator<< / >> for UUID
#include "scripts/ScriptEngine.h"

namespace sfmx
{

namespace {
/** @brief ScriptComponent blob layout version; bump on format changes. */
constexpr uint32 kScriptComponentVersion = 2;  // v2: script by LuaAsset UUID (was a path)
} // namespace

ScriptComponent::ScriptComponent(SceneNode* owner, const UUID& scriptAssetId)
  : ComponentT<ScriptComponent>(owner) {
  setScriptAssetId(scriptAssetId);
}

ScriptComponent::ScriptComponent(SceneNode* owner)
  : ComponentT<ScriptComponent>(owner) {
  // Deferred: no script yet — onDeserialize sets the asset id and re-binds.
}

void
ScriptComponent::onUpdate(float deltaTime) {
  if (!m_initialized) {
    return;
  }

  sol::protected_function_result result = m_script(getOwner(), deltaTime);
  if (!result.valid()) {
    const sol::error err = result;
    // TODO: log error
    fprintf(stderr, "[Script] %s: %s\n", m_scriptAssetId.toString().c_str(), err.what());
  }
}

void
ScriptComponent::setScriptAsset(SPtr<LuaAsset> asset) {
  // If handed an asset that isn't decoded yet, bring it up through the
  // AssetManager by its UUID (same resolution path as setScriptAssetId).
  if (nullptr != asset && !asset->isLoaded() && AssetManager::isStarted()) {
    SPtr<LuaAsset> loaded =
        AssetManager::instance().load<LuaAsset>(asset->metadata().uuid);
    if (nullptr != loaded) {
      asset = loaded;
    }
  }

  m_scriptAsset   = asset;
  m_scriptAssetId = (nullptr != asset) ? asset->metadata().uuid : UUID::null();
  m_initialized   = false;

  // Compile + bind the Lua function from the asset's text, when both are running.
  if (nullptr != asset && asset->isLoaded() && ScriptEngine::isStarted()) {
    ScriptEngine::instance().initializeScript(this);
  }
}

void
ScriptComponent::setScriptAssetId(const UUID& id) {
  if (id != UUID::null() && AssetManager::isStarted()) {
    SPtr<LuaAsset> asset = AssetManager::instance().load<LuaAsset>(id);
    if (nullptr != asset) {
      setScriptAsset(asset);  // records m_scriptAssetId from the asset (== id)
      return;
    }
  }
  // Couldn't resolve (no manager, null id, or not cataloged): keep the id so it
  // still re-serializes and can resolve later.
  m_scriptAssetId = id;
}

void
ScriptComponent::onSerialize(DataStream& stream) const {
  stream << kScriptComponentVersion;
  stream << m_scriptAssetId;
}

void
ScriptComponent::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != kScriptComponentVersion) {
    return;  // unknown version: leave defaults rather than misread bytes
  }

  UUID id;
  stream >> id;
  // Re-resolve the script by UUID; this (re)binds the function when the asset loads.
  setScriptAssetId(id);
}

} // namespace sfmx
