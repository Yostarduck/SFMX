#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderStates.hpp>

#include "assets/MaterialData.h"
#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

class ShaderAsset;

/**
 * @brief Binds a shader (and its uniforms) to the drawable on the same node.
 *
 * A material is data: a shader reference plus a fixed set of uniform values, held
 * in a @ref MaterialData payload. Keeping that payload separate means a future
 * @ref MaterialAsset can share the exact same serialized form while this
 * component keeps its data inline.
 *
 * A material is long-lived (created at level load), so registering uniforms in
 * setup may allocate — but @ref apply, run every frame, allocates nothing: uniform
 * names are stored once as @c String members and passed to @c sf::Shader::setUniform
 * by reference. The material owns no drawing of its own; a drawable component
 * (@ref SpriteComponent, @ref ParticleSystemComponent) holds a pointer to it and
 * calls @ref apply on its own copy of @c sf::RenderStates before drawing — which is
 * why a material only affects the sibling that opted into it, never the whole node.
 *
 * Serialization stores the shader by asset UUID and re-resolves it through the
 * AssetManager, mirroring @ref SpriteComponent and @ref ParticleSystemComponent.
 */
class SFMX_UTILITY_EXPORT MaterialComponent : public ComponentT<MaterialComponent>
{
 public:
  /** @brief How many bytes of a uniform value are meaningful (by kind). */
  using UniformKind = MaterialUniformKind;

  static constexpr size_t kMaxUniforms = MaterialData::kMaxUniforms;

  explicit MaterialComponent(SceneNode* owner);

  /** @brief The shader this material binds; keeps the asset alive. */
  void
  setShader(SPtr<ShaderAsset> shader);

  /**
   * @brief Record the shader asset UUID and resolve it via AssetManager when
   *        available; if it can't be resolved the id is still kept so it
   *        re-serializes and can resolve later.
   */
  void
  setShaderAssetId(const UUID& id);

  /** @brief UUID of the referenced ShaderAsset, or UUID::null() for none. */
  NODISCARD const UUID&
  getShaderAssetId() const { return m_data.shaderAssetId; }

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
  setBindCurrentTexture(bool bind) { m_data.bindCurrentTexture = bind; }

  /** @brief Whether the drawable's texture is bound to the sampler uniform. */
  NODISCARD FORCEINLINE bool
  getBindCurrentTexture() const { return m_data.bindCurrentTexture; }

  /** @brief Name of the @c sampler2D uniform bound to the drawable's texture. */
  void
  setTextureUniform(StringView name);

  /** @brief The @c sampler2D uniform name bound to the drawable's texture. */
  NODISCARD FORCEINLINE const String&
  getTextureUniform() const { return m_data.textureUniform; }

  /** @brief The serialized material payload (shader id + uniforms). */
  NODISCARD FORCEINLINE const MaterialData&
  getData() const { return m_data; }

  /**
   * @brief Upload the material's uniforms and set @c states.shader to this shader.
   *
   * Called by the drawable's @ref onDraw before @c target.draw. No-op (leaves the
   * states untouched) until the shader asset is loaded. Allocation-free.
   */
  void
  apply(sf::RenderStates& states) const;

  /** @brief Writes the shader UUID and the uniform payload. */
  void
  onSerialize(DataStream& stream) const override;
  /** @brief Restores the payload and re-resolves the shader by UUID. */
  void
  onDeserialize(DataStream& stream) override;

 private:
  // Find a slot by name, or reserve the next free one and stamp its name. Returns
  // nullptr only when the fixed uniform budget is exhausted.
  MaterialUniform*
  slotFor(StringView name, UniformKind kind);

  SPtr<ShaderAsset> m_shaderAsset;
  MaterialData      m_data;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::MaterialComponent)
