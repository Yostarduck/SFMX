#pragma once

#include <cstddef>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/System/Vector2.hpp>

#include "gfx/GLFunctions.h"

namespace sf
{
class RenderTarget;
class Texture;
class VertexBuffer;
} // namespace sf

namespace sfmx::gfx
{

/**
 * @brief Per-instance record for one quad.
 *
 * Deliberately aliased to @c sf::Vertex: that type is exactly 20 bytes with its
 * members at offsets 0 / 8 / 12 (SFML's own render path hard-codes those), which
 * is precisely the layout an instance needs. Reusing it means the stream can ride
 * in an @c sf::VertexBuffer, so SFML keeps managing the GL buffer's context
 * lifetime and we never touch @c glGenBuffers or @c glDeleteBuffers.
 *
 * The fields are reinterpreted as:
 * - @c position  centre of the quad, in the space @ref QuadBatch::mvp maps from.
 * - @c color     tint, multiplied with the texture sample.
 * - @c texCoords @c (rotation in radians, blend factor in 0..1).
 */
using QuadInstance = sf::Vertex;

/** @brief Builds a @ref QuadInstance from its logical fields. */
[[nodiscard]] inline QuadInstance
makeQuadInstance(const sf::Vector2f& centre,
                 const sf::Color&    tint,
                 float               rotation,
                 float               blend) {
  return QuadInstance{centre, tint, {rotation, blend}};
}

/** @brief Everything one instanced draw call needs. */
struct QuadBatch
{
  /** @brief Stream of @ref QuadInstance, one element per quad. */
  const sf::VertexBuffer* instances     = nullptr;
  /** @brief Number of leading elements of @ref instances to draw. */
  std::size_t             instanceCount = 0;
  /** @brief View projection times the world transform of whatever owns the quads. */
  sf::Transform           mvp;
  /** @brief Sampled at the quad's 0..1 UVs; null draws untextured. */
  const sf::Texture*      texture       = nullptr;
  sf::BlendMode           blendMode     = sf::BlendAlpha;
  /** @brief Full quad size at blend 0; the shader mixes towards @ref sizeAtBlend1. */
  sf::Vector2f            sizeAtBlend0  = {0.f, 0.f};
  /** @brief Full quad size at blend 1. */
  sf::Vector2f            sizeAtBlend1  = {0.f, 0.f};
};

/**
 * @brief Draws textured, rotated, tinted quads with one instanced draw call.
 *
 * Quad geometry is generated from @c gl_VertexID in the vertex shader, so there
 * is no geometry buffer at all: the only thing uploaded per frame is the caller's
 * instance stream, at 20 bytes per quad.
 *
 * Share one instance across all callers -- it owns the program. Initialisation is
 * deferred to the first @ref draw because resolving GL entry points requires a
 * current context, which only a live render target guarantees.
 */
class InstancedQuadRenderer
{
 public:
  InstancedQuadRenderer() = default;
  ~InstancedQuadRenderer() = default;

  InstancedQuadRenderer(const InstancedQuadRenderer&) = delete;
  InstancedQuadRenderer& operator=(const InstancedQuadRenderer&) = delete;

  /**
   * @brief Whether instanced drawing is usable.
   *
   * Only meaningful after the first @ref draw; before that the capability probe
   * has not run yet and this returns false.
   */
  [[nodiscard]] bool
  isSupported() const { return m_supported; }

  /**
   * @brief Draws @p batch onto @p target with a single @c glDrawArraysInstanced.
   *
   * Leaves @p target's SFML state cache consistent on return, so the caller can
   * keep issuing ordinary @c target.draw calls afterwards.
   *
   * @return False when instancing is unavailable or the batch is unusable, in
   *         which case nothing was drawn and the caller should fall back.
   */
  bool
  draw(sf::RenderTarget& target, const QuadBatch& batch);

 private:
  /** @brief Loads the entry points and compiles the program, once. */
  bool
  initialize();

  /** @brief Translates @p mode into @c glBlendFuncSeparate / equation calls. */
  void
  applyBlendMode(const sf::BlendMode& mode) const;

  /** @brief Mirrors @c RenderTarget::applyCurrentView's viewport maths. */
  void
  applyViewport(const sf::RenderTarget& target) const;

  sf::Shader  m_shader;
  GLFunctions m_gl;
  bool        m_initialized = false;
  bool        m_supported   = false;
};

} // namespace sfmx::gfx
