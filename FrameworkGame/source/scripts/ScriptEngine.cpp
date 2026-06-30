#include "scripts/ScriptEngine.h"

#include "scripts/RegisterAll.h"
#include "scene/ScriptComponent.h"

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
  const std::string& scriptName = scriptComponent->m_scriptName;

  const sol::protected_function* chunk = getCompiledChunk(scriptName);
  if (nullptr == chunk) {
    return;  // load failed; the error was already logged
  }

  sol::protected_function_result returned = (*chunk)();
  if (!returned.valid()) {
    const sol::error err = returned;
    fprintf(stderr, "[Script] init %s: %s\n", scriptName.c_str(), err.what());
    return;
  }

  if (sol::type::table != returned.get_type()) {
    fprintf(stderr,
            "[Script] %s must return a table of lifecycle hooks\n",
            scriptName.c_str());
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
ScriptEngine::getCompiledChunk(const std::string& scriptName) {
  auto found = m_scriptCache.find(scriptName);
  if (found != m_scriptCache.end()) {
    return &found->second;
  }

  // First sighting of this script: read + compile once and cache the chunk.
  sol::load_result chunk = m_lua.load_file(scriptName);
  if (!chunk.valid()) {
    const sol::error err = chunk;
    fprintf(stderr, "[Script] load %s: %s\n", scriptName.c_str(), err.what());
    return nullptr;
  }

  auto inserted =
    m_scriptCache.emplace(scriptName, sol::protected_function(chunk));
  return &inserted.first->second;
}

} // namespace sfmx