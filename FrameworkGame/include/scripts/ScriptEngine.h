#pragma once

#include "core/platform/Prerequisites.h"
#include "utils/Module.h"

#include <string>
#include <unordered_map>

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

  /** @brief Returns the compiled chunk for @p scriptName, reading and compiling
   *         the file on first use and caching it thereafter. Each call to the
   *         returned chunk produces a fresh hooks table with its own upvalues,
   *         so nodes sharing a script stay isolated while the disk read +
   *         compile happens only once per file. Returns @c nullptr if the file
   *         fails to load (the error is logged).
   */
  NODISCARD const sol::protected_function*
  getCompiledChunk(const std::string& scriptName);

  sol::state m_lua;

  // Compiled-chunk cache keyed by script path; see getCompiledChunk.
  std::unordered_map<std::string, sol::protected_function> m_scriptCache;
};

} // namespace sfmx