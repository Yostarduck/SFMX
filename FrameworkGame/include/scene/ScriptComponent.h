#pragma once

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"

#include <sol/sol.hpp>

namespace sfmx
{

class ScriptEngine;

class ScriptComponent : public ComponentT<ScriptComponent>
{
 public:
  ScriptComponent(SceneNode* owner, std::string_view scriptName);

  // Run the script each frame, passing the owning node in as `self`.
  void
  onUpdate(float deltaTime) override;

  NODISCARD FORCEINLINE bool
  isInitialized() const { return m_initialized; }

 private:
  friend ScriptEngine;

  String m_scriptName;
  sol::protected_function m_script;
  bool m_initialized = false;
};

}