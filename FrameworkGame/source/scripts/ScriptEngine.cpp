#include "scripts/ScriptEngine.h"

#include "scripts/RegisterAll.h"
#include "scene/ScriptComponent.h"

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
  const std::string& scriptName = scriptComponent->m_scriptName;

  // TODO: log errors
  sol::load_result chunk = m_lua.load_file(scriptName);
  if (!chunk.valid()) {
    const sol::error err = chunk;
    fprintf(stderr, "[Script] load %s: %s\n", scriptName.c_str(), err.what());
    return;
  }

  sol::protected_function_result returned = chunk();
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

} // namespace sfmx