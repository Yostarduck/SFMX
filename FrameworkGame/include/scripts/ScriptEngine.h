#pragma once

#include "core/platform/Prerequisites.h"
#include "utils/Module.h"

#include <sol/sol.hpp>

namespace sfmx
{

class ScriptComponent;

class ScriptEngine : public Module<ScriptEngine>
{
 public:
  void
  initializeScript(ScriptComponent* scriptComponent);

 protected:
  void
  onStartUp() override;

 private:
  friend class Module<ScriptEngine>;

  ScriptEngine() = default;

  void
  loadScript(ScriptComponent* scriptComponent);

  sol::state m_lua;
};

} // namespace sfmx