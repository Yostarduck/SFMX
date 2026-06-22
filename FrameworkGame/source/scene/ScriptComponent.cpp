#include "scene/ScriptComponent.h"
#include "scene/SceneNode.h"

#include "scripts/ScriptEngine.h"

namespace sfmx
{

ScriptComponent::ScriptComponent(SceneNode* owner, std::string_view scriptName)
  : ComponentT<ScriptComponent>(owner),
    m_scriptName(scriptName),
    m_initialized(false) {
  ScriptEngine::instance().initializeScript(this);
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
    fprintf(stderr, "[Script] %s: %s\n", m_scriptName.c_str(), err.what());
  }
}

} // namespace sfmx