#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderStates.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

class ShaderAsset;

/**
 * @brief Binds a shader (and its uniforms) to the drawable on the same node.
 *
 * A material is long-lived (created at level load), so registering uniforms in
 * setup may allocate — but @ref apply, run every frame, allocates nothing: uniform
 * names are stored once as @c String members and passed to @c sf::Shader::setUniform
 * by reference. The material owns no drawing of its own; a drawable component
 * (@ref SpriteComponent, @ref ParticleSystemComponent) holds a pointer to it and
 * calls @ref apply on its own copy of @c sf::RenderStates before drawing — which is
 * why a material only affects the sibling that opted into it, never the whole node.
 */
class SFMX_UTILITY_EXPORT MaterialComponent : public ComponentT<MaterialComponent>
{
 public:
  /** @brief How many bytes of a uniform value are meaningful (by kind). */
  enum class UniformKind : uint8 { kFloat, kVec2, kVec3, kVec4 };

  static constexpr size_t kMaxUniforms = 16;

  explicit MaterialComponent(SceneNode* owner);

  /** @brief The shader this material binds; keeps the asset alive. */
  void
  setShader(SPtr<ShaderAsset> shader);

  /** @brief The bound shader asset, or nullptr if none set. */
  NODISCARD FORCEINLINE const SPtr<ShaderAsset>&
  getShader() const { return m_shaderAsset; }

  // Register or update a uniform. Registering a new name is setup (may allocate the
  // name string); updating an existing one only rewrites its value (allocation-free,
  // safe to call every frame).
  void
  setFloat(StringView name, float value);
  void
  setVec2(StringView name, float x, float y);
  void
  setVec3(StringView name, float x, float y, float z);
  void
  setColor(StringView name, const sf::Color& color);

  /** @brief Whether @ref apply binds the drawable's own texture to a @c sampler2D
   *         uniform (default: true, named "u_texture"). Turn off for shaders that
   *         sample no texture, to avoid an "uniform not found" warning. */
  FORCEINLINE void
  setBindCurrentTexture(bool bind) { m_bindCurrentTexture = bind; }

  /** @brief Name of the @c sampler2D uniform bound to the drawable's texture. */
  void
  setTextureUniform(StringView name);

  /**
   * @brief Upload the material's uniforms and set @c states.shader to this shader.
   *
   * Called by the drawable's @ref onDraw before @c target.draw. No-op (leaves the
   * states untouched) until the shader asset is loaded. Allocation-free.
   */
  void
  apply(sf::RenderStates& states) const;

 private:
  struct UniformSlot
  {
    String      name;                          // stored once; passed by ref in apply
    UniformKind kind     = UniformKind::kFloat;
    float       value[4] = {0.f, 0.f, 0.f, 0.f};
  };

  // Find a slot by name, or reserve the next free one and stamp its name. Returns
  // nullptr only when the fixed uniform budget is exhausted.
  UniformSlot*
  slotFor(StringView name, UniformKind kind);

  SPtr<ShaderAsset>              m_shaderAsset;
  Array<UniformSlot, kMaxUniforms> m_uniforms;
  size_t                        m_uniformCount = 0;
  String                        m_textureUniform = "u_texture";  // bound once
  bool                          m_bindCurrentTexture = true;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::MaterialComponent)
