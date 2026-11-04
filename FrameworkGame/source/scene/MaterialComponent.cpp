#include "scene/MaterialComponent.h"

#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/Shader.hpp>

#include "assets/AssetManager.h"
#include "assets/ShaderAsset.h"

namespace sfmx
{

MaterialComponent::MaterialComponent(SceneNode* owner)
  : ComponentT<MaterialComponent>(owner) {}

void
MaterialComponent::setShader(SPtr<ShaderAsset> shader) {
  m_shaderAsset        = std::move(shader);
  m_data.shaderAssetId = (nullptr != m_shaderAsset)
                             ? m_shaderAsset->metadata().uuid
                             : UUID::null();
}

void
MaterialComponent::setShaderAssetId(const UUID& id) {
  if (id != UUID::null() && AssetManager::isStarted()) {
    SPtr<ShaderAsset> asset = AssetManager::instance().load<ShaderAsset>(id);
    if (nullptr != asset) {
      setShader(std::move(asset));  // records the asset's uuid (== id)
      return;
    }
  }

  // Couldn't resolve (no manager, null id, or not cataloged): keep the id so it
  // still re-serializes and can resolve later, and drop any stale shader.
  m_shaderAsset.reset();
  m_data.shaderAssetId = id;
}

MaterialUniform*
MaterialComponent::slotFor(StringView name, UniformKind kind) {
  for (MaterialUniform& uniform : m_data.uniforms) {
    if (uniform.name == name) {
      uniform.kind = kind;
      return &uniform;
    }
  }
  if (m_data.uniforms.size() >= kMaxUniforms) {
    return nullptr;  // fixed budget exhausted; caller's update is dropped
  }
  MaterialUniform& slot = m_data.uniforms.emplace_back();
  slot.name = String(name);  // setup-time: the only place a name string is written
  slot.kind = kind;
  return &slot;
}

void
MaterialComponent::setFloat(StringView name, float value) {
  if (MaterialUniform* s = slotFor(name, UniformKind::kFloat)) {
    s->value[0] = value;
  }
}

void
MaterialComponent::setVec2(StringView name, float x, float y) {
  if (MaterialUniform* s = slotFor(name, UniformKind::kVec2)) {
    s->value[0] = x;
    s->value[1] = y;
  }
}

void
MaterialComponent::setVec3(StringView name, float x, float y, float z) {
  if (MaterialUniform* s = slotFor(name, UniformKind::kVec3)) {
    s->value[0] = x;
    s->value[1] = y;
    s->value[2] = z;
  }
}

void
MaterialComponent::setColor(StringView name, const sf::Color& color) {
  if (MaterialUniform* s = slotFor(name, UniformKind::kVec4)) {
    s->value[0] = color.r / 255.f;
    s->value[1] = color.g / 255.f;
    s->value[2] = color.b / 255.f;
    s->value[3] = color.a / 255.f;
  }
}

void
MaterialComponent::setTextureUniform(StringView name) {
  m_data.textureUniform = String(name);
}

void
MaterialComponent::apply(sf::RenderStates& states) const {
  if (!m_shaderAsset || !m_shaderAsset->isLoaded()) {
    return;
  }

  sf::Shader& shader = m_shaderAsset->shader();
  if (m_data.bindCurrentTexture) {
    // Bind the drawable's own texture (set on states at draw time) to the sampler.
    shader.setUniform(m_data.textureUniform, sf::Shader::CurrentTexture);
  }
  for (const MaterialUniform& u : m_data.uniforms) {
    switch (u.kind) {
      case UniformKind::kFloat:
        shader.setUniform(u.name, u.value[0]);
        break;
      case UniformKind::kVec2:
        shader.setUniform(u.name, sf::Glsl::Vec2(u.value[0], u.value[1]));
        break;
      case UniformKind::kVec3:
        shader.setUniform(u.name, sf::Glsl::Vec3(u.value[0], u.value[1], u.value[2]));
        break;
      case UniformKind::kVec4:
        shader.setUniform(u.name,
                          sf::Glsl::Vec4(u.value[0], u.value[1], u.value[2], u.value[3]));
        break;
    }
  }

  states.shader = &shader;
}

void
MaterialComponent::onSerialize(DataStream& stream) const {
  m_data.serialize(stream);
}

void
MaterialComponent::onDeserialize(DataStream& stream) {
  if (!m_data.deserialize(stream)) {
    return;  // unknown version / over-budget payload: keep defaults
  }
  // Copy first: setShaderAssetId() may rewrite m_data.shaderAssetId.
  const UUID id = m_data.shaderAssetId;
  setShaderAssetId(id);
}

} // namespace sfmx
