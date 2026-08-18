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

/**
 * @brief Free-form per-instance payload, delivered to the shader in a uniform
 *        buffer and read as @c u_customData[gl_InstanceID].
 *
 * The 16-byte shape is deliberate: std140 pads every uniform-array element up to a
 * 16-byte boundary, so one int plus three floats is exactly one element with no
 * waste and no packing tricks. The GLSL struct must mirror it member for member.
 *
 * The renderer never interprets these fields -- they mean whatever the shader
 * bound through @ref QuadBatch::shader decides they mean.
 */
struct QuadCustomData
{
  std::int32_t id = 0;
  float        x  = 0.f;
  float        y  = 0.f;
  float        z  = 0.f;
};

static_assert(sizeof(QuadCustomData) == 16,
              "QuadCustomData must match one std140 uniform-array element.");

/**
 * @brief Instances per custom-data upload, and therefore per draw call.
 *
 * 16 KiB is the smallest uniform block GL 3.3 guarantees, which at 16 bytes an
 * element is 1024 instances. Batches larger than this are split across several
 * draws rather than clamped. Kept a fixed constant rather than derived from
 * @c GL_MAX_UNIFORM_BLOCK_SIZE because every shader has to declare the same array
 * length -- change this and every custom particle shader has to change with it.
 */
constexpr std::size_t kMaxCustomDataInstances = 1024;

/** @brief Name of the uniform block a shader declares to receive custom data. */
constexpr const char* kCustomDataBlockName = "ParticleCustomData";

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

  /**
   * @brief Replaces the built-in program. Null draws with the built-in one.
   *
   * A replacement must supply a *vertex* stage as well as a fragment one: a
   * fragment-only shader leaves the fixed-function vertex pipeline in charge,
   * which cannot feed the generic attributes this renderer binds. See the class
   * comment for the contract it has to honour.
   */
  sf::Shader*             shader        = nullptr;

  /**
   * @brief One @ref QuadCustomData per instance, in the same order as
   *        @ref instances. Null skips the uniform buffer entirely.
   */
  const QuadCustomData*   customData    = nullptr;
  /** @brief Elements in @ref customData; should equal @ref instanceCount. */
  std::size_t             customDataCount = 0;
};

/**
 * @brief Draws textured, rotated, tinted quads with one instanced draw call.
 *
 * Quad geometry is generated from @c gl_VertexID in the vertex shader, so there
 * is no geometry buffer at all: the only thing uploaded per frame is the caller's
 * instance stream, at 20 bytes per quad.
 *
 * Share one instance across all callers -- it owns the program and the shared
 * custom-data uniform buffer. Initialisation is deferred to the first @ref draw
 * because resolving GL entry points requires a current context, which only a live
 * render target guarantees.
 *
 * A @ref QuadBatch::shader replacement must match this contract:
 * @code
 * #version 330 core
 * layout(location = 0) in vec2 a_center;
 * layout(location = 1) in vec4 a_color;
 * layout(location = 2) in vec2 a_rotBlend;   // (rotation radians, blend 0..1)
 *
 * uniform mat4 u_mvp;
 * uniform vec2 u_sizeBegin;
 * uniform vec2 u_sizeEnd;
 *
 * // Optional. Declare it to receive QuadBatch::customData; omit it and the
 * // renderer skips the upload. Array length must be kMaxCustomDataInstances.
 * struct ParticleCustom { int id; float x; float y; float z; };
 * layout(std140) uniform ParticleCustomData {
 *   ParticleCustom u_customData[1024];
 * };
 *
 * // ...u_customData[gl_InstanceID] is this quad's payload.
 * @endcode
 * The fragment stage gets @c u_texture (unit 0) and @c u_useTexture. Uniforms a
 * shader does not declare cost one SFML warning each, not one per frame.
 *
 * Batches larger than @ref kMaxCustomDataInstances are split across several draws;
 * @c gl_InstanceID restarts at zero for each, and the matching slice of custom data
 * is uploaded alongside, so the index always addresses the right payload.
 */
class InstancedQuadRenderer
{
 public:
  InstancedQuadRenderer() = default;
  ~InstancedQuadRenderer();

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
  /** @brief Loads the entry points, compiles the program and allocates the UBO. */
  bool
  initialize();

  /** @brief Translates @p mode into @c glBlendFuncSeparate / equation calls. */
  void
  applyBlendMode(const sf::BlendMode& mode) const;

  /** @brief Mirrors @c RenderTarget::applyCurrentView's viewport maths. */
  void
  applyViewport(const sf::RenderTarget& target) const;

  /**
   * @brief The custom-data block's index in @p program, or @c kGlInvalidIndex.
   *
   * Memoised for the last program asked about: the lookup is a driver-side string
   * compare and this runs once per emitter per frame.
   */
  GLuint
  customDataBlockIndex(GLuint program);

  sf::Shader  m_shader;
  GLFunctions m_gl;
  bool        m_initialized = false;
  bool        m_supported   = false;

  /** @brief Shared custom-data uniform buffer, kMaxCustomDataInstances elements. */
  GLuint      m_customDataUbo = 0;
  /** @brief Last program passed to @ref customDataBlockIndex, and its answer. */
  GLuint      m_cachedProgram    = 0;
  GLuint      m_cachedBlockIndex = 0;
};

} // namespace sfmx::gfx
