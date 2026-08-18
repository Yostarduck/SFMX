#include "gfx/InstancedQuadRenderer.h"

#include <cstdint>
#include <iostream>

#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexBuffer.hpp>
#include <SFML/Graphics/View.hpp>

namespace sfmx::gfx
{

namespace
{

/**
 * @brief Expands one @ref QuadInstance into a screen-aligned, rotated quad.
 *
 * The four corners come from @c gl_VertexID rather than a geometry buffer, drawn
 * as a triangle strip in the order (0,0) (1,0) (0,1) (1,1). Winding is
 * inconsistent between the two triangles, which is harmless because face culling
 * is off in every state SFML leaves behind.
 */
constexpr const char* kVertexShader = R"(#version 330 core

layout(location = 0) in vec2 a_center;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_rotBlend;

uniform mat4 u_mvp;
uniform vec2 u_sizeBegin;
uniform vec2 u_sizeEnd;

out vec4 v_color;
out vec2 v_uv;

void main()
{
  vec2  uv    = vec2(float(gl_VertexID & 1), float(gl_VertexID >> 1));
  vec2  size  = mix(u_sizeBegin, u_sizeEnd, a_rotBlend.y);
  vec2  local = (uv * 2.0 - 1.0) * size * 0.5;

  float c = cos(a_rotBlend.x);
  float s = sin(a_rotBlend.x);
  vec2  world = vec2(local.x * c - local.y * s,
                     local.x * s + local.y * c) + a_center;

  gl_Position = u_mvp * vec4(world, 0.0, 1.0);
  v_color     = a_color;
  v_uv        = uv;
}
)";

/**
 * @brief Tints the quad, sampling the texture only when one is bound.
 *
 * SFML's fixed-function path got the untextured case for free by disabling
 * GL_TEXTURE_2D; with our own program the branch has to be explicit, otherwise an
 * unbound sampler would return black.
 */
constexpr const char* kFragmentShader = R"(#version 330 core

in vec4 v_color;
in vec2 v_uv;

uniform sampler2D u_texture;
uniform bool      u_useTexture;

out vec4 fragColor;

void main()
{
  fragColor = u_useTexture ? v_color * texture(u_texture, v_uv) : v_color;
}
)";

// Attribute locations, matched to the layout qualifiers above.
constexpr GLuint kCenterLocation   = 0;
constexpr GLuint kColorLocation    = 1;
constexpr GLuint kRotBlendLocation = 2;

// Byte offsets of sf::Vertex's members. SFML's own RenderTarget hard-codes the
// same three values, so they are as stable as the type itself.
constexpr std::uintptr_t kCenterOffset   = 0;
constexpr std::uintptr_t kColorOffset    = 8;
constexpr std::uintptr_t kRotBlendOffset = 12;

/** @brief Number of corners the strip draws per instance. */
constexpr GLsizei kVerticesPerQuad = 4;

/** @brief Turns a byte offset into the pointer @c glVertexAttribPointer wants. */
[[nodiscard]] const void*
bufferOffset(std::uintptr_t offset) {
  return reinterpret_cast<const void*>(offset);
}

/** @brief Mirrors SFML's own blend-factor translation. */
[[nodiscard]] GLenum
toGlFactor(sf::BlendMode::Factor factor) {
  switch (factor) {
    case sf::BlendMode::Factor::Zero:             return kGlZero;
    case sf::BlendMode::Factor::One:              return kGlOne;
    case sf::BlendMode::Factor::SrcColor:         return kGlSrcColor;
    case sf::BlendMode::Factor::OneMinusSrcColor: return kGlOneMinusSrcColor;
    case sf::BlendMode::Factor::DstColor:         return kGlDstColor;
    case sf::BlendMode::Factor::OneMinusDstColor: return kGlOneMinusDstColor;
    case sf::BlendMode::Factor::SrcAlpha:         return kGlSrcAlpha;
    case sf::BlendMode::Factor::OneMinusSrcAlpha: return kGlOneMinusSrcAlpha;
    case sf::BlendMode::Factor::DstAlpha:         return kGlDstAlpha;
    case sf::BlendMode::Factor::OneMinusDstAlpha: return kGlOneMinusDstAlpha;
  }
  return kGlZero;
}

/** @brief Mirrors SFML's own blend-equation translation. */
[[nodiscard]] GLenum
toGlEquation(sf::BlendMode::Equation equation) {
  switch (equation) {
    case sf::BlendMode::Equation::Add:             return kGlFuncAdd;
    case sf::BlendMode::Equation::Subtract:        return kGlFuncSubtract;
    case sf::BlendMode::Equation::ReverseSubtract: return kGlFuncReverseSubtract;
    case sf::BlendMode::Equation::Min:             return kGlMin;
    case sf::BlendMode::Equation::Max:             return kGlMax;
  }
  return kGlFuncAdd;
}

} // namespace

bool
InstancedQuadRenderer::initialize() {
  if (m_initialized) {
    return m_supported;
  }
  m_initialized = true;

  if (!sf::Shader::isAvailable() || !sf::VertexBuffer::isAvailable()) {
    std::cerr << "InstancedQuadRenderer: shaders or vertex buffers unavailable, "
                 "instanced drawing disabled" << std::endl;
    return false;
  }

  if (!loadGLFunctions(m_gl)) {
    std::cerr << "InstancedQuadRenderer: driver is missing the instanced-array "
                 "entry points, instanced drawing disabled" << std::endl;
    return false;
  }

  if (!m_shader.loadFromMemory(kVertexShader, kFragmentShader)) {
    std::cerr << "InstancedQuadRenderer: failed to compile the quad shader, "
                 "instanced drawing disabled" << std::endl;
    return false;
  }

  // Records the sampler's location so every Shader::bind points it at unit 0,
  // which is where we bind the batch texture.
  m_shader.setUniform("u_texture", sf::Shader::CurrentTexture);

  m_supported = true;
  std::cout << "InstancedQuadRenderer: instanced quad drawing enabled"
            << std::endl;
  return true;
}

void
InstancedQuadRenderer::applyBlendMode(const sf::BlendMode& mode) const {
  m_gl.enable(kGlBlend);
  m_gl.blendFuncSeparate(toGlFactor(mode.colorSrcFactor),
                         toGlFactor(mode.colorDstFactor),
                         toGlFactor(mode.alphaSrcFactor),
                         toGlFactor(mode.alphaDstFactor));
  m_gl.blendEquationSeparate(toGlEquation(mode.colorEquation),
                             toGlEquation(mode.alphaEquation));
}

void
InstancedQuadRenderer::applyViewport(const sf::RenderTarget& target) const {
  // SFML applies the viewport lazily, on its next draw. A batch that is the first
  // thing drawn under a freshly set view would otherwise inherit the previous
  // view's viewport, so set it here exactly the way SFML would.
  const sf::IntRect viewport = target.getViewport(target.getView());
  const GLint top = static_cast<GLint>(target.getSize().y) -
                    (viewport.position.y + viewport.size.y);

  m_gl.viewport(viewport.position.x, top, viewport.size.x, viewport.size.y);
}

bool
InstancedQuadRenderer::draw(sf::RenderTarget& target, const QuadBatch& batch) {
  if (nullptr == batch.instances || 0 == batch.instanceCount) {
    return false;
  }

  if (!target.setActive(true)) {
    return false;
  }

  if (!initialize()) {
    return false;
  }

  applyViewport(target);

  m_shader.setUniform("u_mvp", sf::Glsl::Mat4(batch.mvp));
  m_shader.setUniform("u_sizeBegin", batch.sizeAtBlend0);
  m_shader.setUniform("u_sizeEnd", batch.sizeAtBlend1);
  m_shader.setUniform("u_useTexture", nullptr != batch.texture);
  sf::Shader::bind(&m_shader);

  applyBlendMode(batch.blendMode);

  if (nullptr != batch.texture) {
    sf::Texture::bind(batch.texture);
  }

  sf::VertexBuffer::bind(batch.instances);

  const GLsizei stride = static_cast<GLsizei>(sizeof(QuadInstance));

  m_gl.vertexAttribPointer(kCenterLocation, 2, kGlFloat, kGlFalse,
                           stride, bufferOffset(kCenterOffset));
  m_gl.vertexAttribPointer(kColorLocation, 4, kGlUnsignedByte, kGlTrue,
                           stride, bufferOffset(kColorOffset));
  m_gl.vertexAttribPointer(kRotBlendLocation, 2, kGlFloat, kGlFalse,
                           stride, bufferOffset(kRotBlendOffset));

  m_gl.vertexAttribDivisor(kCenterLocation, 1);
  m_gl.vertexAttribDivisor(kColorLocation, 1);
  m_gl.vertexAttribDivisor(kRotBlendLocation, 1);

  m_gl.enableVertexAttribArray(kCenterLocation);
  m_gl.enableVertexAttribArray(kColorLocation);
  m_gl.enableVertexAttribArray(kRotBlendLocation);

  m_gl.drawArraysInstanced(kGlTriangleStrip, 0, kVerticesPerQuad,
                           static_cast<GLsizei>(batch.instanceCount));

  // Divisors are global while no vertex array object is bound, so put them back.
  m_gl.disableVertexAttribArray(kCenterLocation);
  m_gl.disableVertexAttribArray(kColorLocation);
  m_gl.disableVertexAttribArray(kRotBlendLocation);
  m_gl.vertexAttribDivisor(kCenterLocation, 0);
  m_gl.vertexAttribDivisor(kColorLocation, 0);
  m_gl.vertexAttribDivisor(kRotBlendLocation, 0);

  // Mandatory: the target caches the blend mode, texture, shader and view it
  // believes are current. Without this the next target.draw would skip
  // re-applying state we changed behind its back.
  target.resetGLStates();

  return true;
}

} // namespace sfmx::gfx
