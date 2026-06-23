#include "scene/ScriptComponent.h"
#include "scene/SceneNode.h"

#include "core/DataStream.h"
#include "scripts/ScriptEngine.h"

namespace sfmx
{

namespace {
/** @brief ScriptComponent blob layout version; bump on format changes. */
constexpr uint32 kScriptComponentVersion = 1;
} // namespace

ScriptComponent::ScriptComponent(SceneNode* owner, std::string_view scriptName)
  : ComponentT<ScriptComponent>(owner),
    m_scriptName(scriptName),
    m_initialized(false) {
  // Guarded so a ScriptComponent can be constructed (e.g. for serialization or
  // headless tooling) even when the scripting module isn't running.
  if (ScriptEngine::isStarted()) {
    ScriptEngine::instance().initializeScript(this);
  }
}

ScriptComponent::ScriptComponent(SceneNode* owner)
  : ComponentT<ScriptComponent>(owner),
    m_scriptName(),
    m_initialized(false) {
  // Deferred: no script yet — onDeserialize sets the name and re-binds.
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

void
ScriptComponent::onSerialize(DataStream& stream) const {
  stream << kScriptComponentVersion;
  stream.writeString(m_scriptName);
}

void
ScriptComponent::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != kScriptComponentVersion) {
    return;  // unknown version: leave defaults rather than misread bytes
  }

  m_scriptName  = stream.readString();
  m_initialized = false;
  // Re-bind the Lua function from the name, when the ScriptEngine is available.
  if (!m_scriptName.empty() && ScriptEngine::isStarted()) {
    ScriptEngine::instance().initializeScript(this);
  }
}

} // namespace sfmx