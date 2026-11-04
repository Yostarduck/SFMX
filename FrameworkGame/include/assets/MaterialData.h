#pragma once

#include "core/platform/Prerequisites.h"
#include "utils/UUID.h"

namespace sfmx
{

class DataStream;

/** @brief How many of a uniform's four value slots are meaningful. */
enum class MaterialUniformKind : uint8 {
  kFloat = 0,
  kVec2  = 1,
  kVec3  = 2,
  kVec4  = 3,
};

/**
 * @brief One shader uniform: its name, kind, and up to four float values.
 *
 * The name is stored once as a @c String so a per-frame @ref apply can pass it to
 * @c sf::Shader::setUniform by reference without allocating.
 */
struct MaterialUniform
{
  String              name;
  MaterialUniformKind kind     = MaterialUniformKind::kFloat;
  float               value[4] = {0.f, 0.f, 0.f, 0.f};
};

/**
 * @brief The serializable payload of a material.
 *
 * A material is data, not a prefab: it is the shader reference (by asset UUID)
 * plus the uniform values uploaded to it. Keeping the payload separate from
 * @ref MaterialComponent lets a future @ref MaterialAsset share the exact same
 * on-disk format while a component keeps the data inline.
 *
 * The fixed @ref kMaxUniforms budget keeps @ref MaterialComponent's per-frame
 * upload allocation-free; @ref deserialize rejects a payload that exceeds it.
 */
struct SFMX_UTILITY_EXPORT MaterialData
{
  static constexpr size_t kMaxUniforms = 16;

  /** @brief Format version; bump on layout changes. */
  static constexpr uint32 kVersion = 1;

  UUID                    shaderAssetId = UUID::null();
  Vector<MaterialUniform> uniforms;
  String                  textureUniform = "u_texture";
  bool                    bindCurrentTexture = true;

  /** @brief Write the payload (versioned) to @p stream. */
  void
  serialize(DataStream& stream) const;

  /**
   * @brief Read a payload written by @ref serialize.
   *
   * @return False on an unknown version or an over-budget uniform count; the
   *         caller should then leave the material at its defaults. Safe because
   *         a component deserializes from its own byte slice.
   */
  bool
  deserialize(DataStream& stream);
};

} // namespace sfmx
