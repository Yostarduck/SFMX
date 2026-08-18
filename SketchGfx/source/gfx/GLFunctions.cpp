#include "gfx/GLFunctions.h"

#include <SFML/Window/Context.hpp>

namespace sfmx::gfx
{

namespace
{

/** @brief Resolves one entry point, clearing @p ok when the driver lacks it. */
template<typename Fn>
void
resolve(Fn& target, const char* name, bool& ok) {
  target = reinterpret_cast<Fn>(sf::Context::getFunction(name));
  if (nullptr == target) {
    ok = false;
  }
}

} // namespace

bool
loadGLFunctions(GLFunctions& functions) {
  bool ok = true;

  // glViewport / glEnable are GL 1.1 and are not exported by wglGetProcAddress,
  // but SFML's getFunction falls back to opengl32.dll for exactly this case.
  resolve(functions.viewport, "glViewport", ok);
  resolve(functions.enable, "glEnable", ok);

  resolve(functions.blendFuncSeparate, "glBlendFuncSeparate", ok);
  resolve(functions.blendEquationSeparate, "glBlendEquationSeparate", ok);

  resolve(functions.enableVertexAttribArray, "glEnableVertexAttribArray", ok);
  resolve(functions.disableVertexAttribArray, "glDisableVertexAttribArray", ok);
  resolve(functions.vertexAttribPointer, "glVertexAttribPointer", ok);

  // The two that actually gate instancing: GL 3.3 core, or ARB_instanced_arrays
  // plus ARB_draw_instanced on a 3.0-era driver.
  resolve(functions.vertexAttribDivisor, "glVertexAttribDivisor", ok);
  resolve(functions.drawArraysInstanced, "glDrawArraysInstanced", ok);

  return ok;
}

} // namespace sfmx::gfx
