#include "scripts/ScriptEngine.h"

#include "scripts/RegisterAll.h"
#include "scene/ScriptComponent.h"
#include "assets/LuaAsset.h"   // the script text comes from a LuaAsset now

namespace sfmx
{

void
ScriptEngine::onStartUp() {
  script::registerAll(m_lua);
}

void
ScriptEngine::initializeScript(ScriptComponent* scriptComponent) {
  if (nullptr != scriptComponent) {
    loadScript(scriptComponent);
  }
}

void
ScriptEngine::loadScript(ScriptComponent* scriptComponent) {
  // The script lives in a LuaAsset (referenced by UUID); nothing to bind until it
  // resolves. Compile from its in-memory text, not from a file on disk.
  const SPtr<LuaAsset>& asset = scriptComponent->m_scriptAsset;
  if (nullptr == asset || !asset->isLoaded()) {
    return;
  }
  const String label = scriptComponent->m_scriptAssetId.toString();

  // TODO: log errors
  sol::load_result chunk = m_lua.load(asset->script());
  if (!chunk.valid()) {
    const sol::error err = chunk;
    fprintf(stderr, "[Script] load %s: %s\n", label.c_str(), err.what());
    return;
  }

  sol::protected_function_result returned = chunk();
  if (!returned.valid()) {
    const sol::error err = returned;
    fprintf(stderr, "[Script] init %s: %s\n", label.c_str(), err.what());
    return;
  }

  if (sol::type::function != returned.get_type()) {
    fprintf(stderr, "[Script] %s must return a function\n", label.c_str());
    return;
  }

  scriptComponent->m_script = returned;
  scriptComponent->m_initialized = true;
}

} // namespace sfmx