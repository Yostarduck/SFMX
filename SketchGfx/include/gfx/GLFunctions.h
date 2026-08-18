#pragma once

#include <cstdint>

namespace sfmx::gfx
{

/**
 * @brief Calling convention every OpenGL entry point uses.
 *
 * The Win32 ABI declares GL functions @c __stdcall; getting this wrong silently
 * corrupts the stack on x86 builds (x64 has a single convention, so the bug only
 * shows up on the 32-bit target).
 */
#if defined(_WIN32)
#  define SFMX_GLAPI __stdcall
#else
#  define SFMX_GLAPI
#endif

// OpenGL scalar types, mirrored here so this header pulls in no GL SDK at all.
using GLenum    = std::uint32_t;
using GLboolean = std::uint8_t;
using GLint     = std::int32_t;
using GLsizei   = std::int32_t;
using GLuint    = std::uint32_t;

// Data types.
constexpr GLenum kGlUnsignedByte = 0x1401;
constexpr GLenum kGlFloat        = 0x1406;

// Primitive types.
constexpr GLenum kGlTriangleStrip = 0x0005;

// Capabilities.
constexpr GLenum kGlBlend = 0x0BE2;

// Blend factors.
constexpr GLenum kGlZero             = 0x0000;
constexpr GLenum kGlOne              = 0x0001;
constexpr GLenum kGlSrcColor         = 0x0300;
constexpr GLenum kGlOneMinusSrcColor = 0x0301;
constexpr GLenum kGlSrcAlpha         = 0x0302;
constexpr GLenum kGlOneMinusSrcAlpha = 0x0303;
constexpr GLenum kGlDstAlpha         = 0x0304;
constexpr GLenum kGlOneMinusDstAlpha = 0x0305;
constexpr GLenum kGlDstColor         = 0x0306;
constexpr GLenum kGlOneMinusDstColor = 0x0307;

// Blend equations.
constexpr GLenum kGlFuncAdd             = 0x8006;
constexpr GLenum kGlMin                 = 0x8007;
constexpr GLenum kGlMax                 = 0x8008;
constexpr GLenum kGlFuncSubtract        = 0x800A;
constexpr GLenum kGlFuncReverseSubtract = 0x800B;

// Booleans.
constexpr GLboolean kGlFalse = 0;
constexpr GLboolean kGlTrue  = 1;

/**
 * @brief The OpenGL entry points instanced drawing needs, resolved at run time.
 *
 * SFML links OpenGL privately and exposes none of these, so they are looked up
 * through @c sf::Context::getFunction. Everything else the renderer needs (buffer
 * creation, shader compilation, texture binding) already has a public SFML API,
 * which is why this table is only nine functions long.
 */
struct GLFunctions
{
  void (SFMX_GLAPI *viewport)(GLint, GLint, GLsizei, GLsizei) = nullptr;
  void (SFMX_GLAPI *enable)(GLenum)                           = nullptr;
  void (SFMX_GLAPI *blendFuncSeparate)(GLenum, GLenum,
                                       GLenum, GLenum)        = nullptr;
  void (SFMX_GLAPI *blendEquationSeparate)(GLenum, GLenum)    = nullptr;
  void (SFMX_GLAPI *enableVertexAttribArray)(GLuint)          = nullptr;
  void (SFMX_GLAPI *disableVertexAttribArray)(GLuint)         = nullptr;
  void (SFMX_GLAPI *vertexAttribPointer)(GLuint, GLint, GLenum, GLboolean,
                                         GLsizei, const void*) = nullptr;
  void (SFMX_GLAPI *vertexAttribDivisor)(GLuint, GLuint)       = nullptr;
  void (SFMX_GLAPI *drawArraysInstanced)(GLenum, GLint,
                                         GLsizei, GLsizei)     = nullptr;
};

/**
 * @brief Resolves every entry point in @p functions through the active context.
 *
 * A context must be current on the calling thread: on Win32 @c getFunction
 * returns null for everything otherwise. Call this lazily from a draw, never at
 * start-up.
 *
 * @param functions Table to fill; left partially populated on failure.
 * @return True when all nine functions resolved, false if the driver is missing
 *         any of them (no GL 3.3 / @c ARB_instanced_arrays).
 */
[[nodiscard]] bool
loadGLFunctions(GLFunctions& functions);

} // namespace sfmx::gfx
