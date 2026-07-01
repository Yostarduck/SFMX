#include "scripts/ScriptEngine.h"

#include "scripts/RegisterAll.h"
#include "scene/ScriptComponent.h"
#include "core/FileSystem.h"   // resolve() the script path against the content root

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
  // Relative script paths resolve under the content root (exe dir at runtime);
  // the component keeps the original relative name so the scene stays portable.
  sol::load_result chunk = m_lua.load_file(FileSystem::resolve(scriptName).string());
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

  if (sol::type::function != returned.get_type()) {
    fprintf(stderr, "[Script] %s must return a function\n", scriptName.c_str());
    return;
  }

  scriptComponent->m_script = returned;
  scriptComponent->m_initialized = true;
}

} // namespace sfmx