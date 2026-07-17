#include "scripts/ScriptEngine.h"

#include "scripts/RegisterAll.h"
#include "scene/ScriptComponent.h"
#include "assets/LuaAsset.h"   // the script text comes from a LuaAsset now

namespace sfmx
{

void
ScriptEngine::onStartUp() {
  script::registerAll(m_lua);
  
  // Pace LuaJIT's incremental GC for the many short-lived userdata that scripts
  // allocate each frame (vectors, angles, transform handles). A larger pause
  // lets the heap grow further between collections, so full cycles run far less
  // often — trading a little memory for smoother frame times under hundreds of
  // scripted entities.
  lua_gc(m_lua.lua_state(), LUA_GCSETPAUSE, 400);
}

void
ScriptEngine::initializeScript(ScriptComponent* scriptComponent) {
  if (nullptr != scriptComponent) {
    loadScript(scriptComponent);
  }
}

void
ScriptEngine::loadScript(ScriptComponent* scriptComponent) {
  if (nullptr == scriptComponent) {
    return;
  }
  
  // The script lives in a LuaAsset (referenced by UUID); nothing to bind until it
  // resolves. Compile from its in-memory text, not from a file on disk.
  const SPtr<LuaAsset>& asset = scriptComponent->m_scriptAsset;
  if (nullptr == asset || !asset->isLoaded()) {
    return;
  }
  const String label = scriptComponent->m_scriptAssetId.toString();

  // On any failure below we return WITHOUT setting m_initialized, so the caller
  // (setScriptAsset reset it to false first) leaves the component disabled — a broken
  // script stops running until a good reload, logged, never crashing.
  const sol::protected_function* chunk = getCompiledChunk(scriptComponent->m_scriptAssetId,
                                                          asset->script());
  if (nullptr == chunk) {
    return;
  }

  sol::protected_function_result returned = (*chunk)();
  if (!returned.valid()) {
    const sol::error err = returned;
    fprintf(stderr, "[Script] init %s: %s (script disabled)\n", label.c_str(), err.what());
    return;
  }

  if (sol::type::table != returned.get_type()) {
    fprintf(stderr, "[Script] %s must return a function (script disabled)\n", label.c_str());
    return;
  }

  const sol::table hooks = returned;
  scriptComponent->m_onCreated   = hooks["onCreated"];
  scriptComponent->m_onStart     = hooks["onStart"];
  scriptComponent->m_onUpdate    = hooks["onUpdate"];
  scriptComponent->m_onDestroyed = hooks["onDestroyed"];

  scriptComponent->m_initialized = true;
}

const sol::protected_function*
ScriptEngine::getCompiledChunk(const UUID label, const std::string& scriptCode) {
  auto found = m_scriptCache.find(label);
  if (found != m_scriptCache.end()) {
    return &found->second;
  }

  // First sighting of this script: read + compile once and cache the chunk.
  sol::load_result chunk = m_lua.load_buffer(scriptCode.c_str(), scriptCode.size());
  if (!chunk.valid()) {
    const sol::error err = chunk;
    fprintf(stderr, "[Script] failed to load script with UUID: %s\n", label.toString().c_str(), err.what());
    return nullptr;
  }

  auto inserted =
    m_scriptCache.emplace(label, sol::protected_function(chunk));
  return &inserted.first->second;
}

} // namespace sfmx