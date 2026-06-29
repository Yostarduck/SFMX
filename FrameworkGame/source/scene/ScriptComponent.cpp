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
    m_scriptName(scriptName) {
      
  if (ScriptEngine::isStarted()) {
    ScriptEngine::instance().initializeScript(this);
  }
}

ScriptComponent::ScriptComponent(SceneNode* owner)
  : ComponentT<ScriptComponent>(owner) {
  // Deferred: no script yet — onDeserialize sets the name and re-binds.
}

ScriptComponent::~ScriptComponent() {
  if (m_initialized && ScriptEngine::isStarted()) {
    callHook(m_onDestroyed);
  }
}

void
ScriptComponent::onAttached() {
  m_linked = true;
  
  triggerOnCreated();
}

void
ScriptComponent::onUpdate(float deltaTime) {
  if (!m_initialized) {
    return;
  }

  if (!m_started) {
    m_started = true;
    callHook(m_onStart);
  }

  if (!m_onUpdate.valid()) {
    return;
  }

  sol::protected_function_result result = m_onUpdate(getOwner(), deltaTime);
  if (!result.valid()) {
    const sol::error err = result;
    // TODO: log error
    fprintf(stderr, "[Script] %s: %s\n", m_scriptName.c_str(), err.what());
  }
}

void
ScriptComponent::triggerOnCreated() {
  if (m_initialized && m_linked && !m_created) {
    m_created = true;
    callHook(m_onCreated);
  }
}

void
ScriptComponent::callHook(const sol::protected_function& fn) {
  if (!fn.valid()) {
    return;
  }

  sol::protected_function_result result = fn(getOwner());
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
  m_created     = false;
  m_started     = false;
  
  if (!m_scriptName.empty() && ScriptEngine::isStarted()) {
    ScriptEngine::instance().initializeScript(this);
    triggerOnCreated();
  }
}

} // namespace sfmx