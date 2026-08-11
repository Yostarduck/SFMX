#include "scene/MaterialComponent.h"

#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/Shader.hpp>

#include "assets/ShaderAsset.h"

namespace sfmx
{

MaterialComponent::MaterialComponent(SceneNode* owner)
  : ComponentT<MaterialComponent>(owner) {}

void
MaterialComponent::setShader(SPtr<ShaderAsset> shader) {
  m_shaderAsset = std::move(shader);
}

MaterialComponent::UniformSlot*
MaterialComponent::slotFor(StringView name, UniformKind kind) {
  for (size_t i = 0; i < m_uniformCount; ++i) {
    if (m_uniforms[i].name == name) {
      m_uniforms[i].kind = kind;
      return &m_uniforms[i];
    }
  }
  if (m_uniformCount >= kMaxUniforms) {
    return nullptr;  // fixed budget exhausted; caller's update is dropped
  }
  UniformSlot& slot = m_uniforms[m_uniformCount++];
  slot.name = String(name);  // setup-time: the only place a name string is written
  slot.kind = kind;
  return &slot;
}

void
MaterialComponent::setFloat(StringView name, float value) {
  if (UniformSlot* s = slotFor(name, UniformKind::kFloat)) {
    s->value[0] = value;
  }
}

void
MaterialComponent::setVec2(StringView name, float x, float y) {
  if (UniformSlot* s = slotFor(name, UniformKind::kVec2)) {
    s->value[0] = x;
    s->value[1] = y;
  }
}

void
MaterialComponent::setVec3(StringView name, float x, float y, float z) {
  if (UniformSlot* s = slotFor(name, UniformKind::kVec3)) {
    s->value[0] = x;
    s->value[1] = y;
    s->value[2] = z;
  }
}

void
MaterialComponent::setColor(StringView name, const sf::Color& color) {
  if (UniformSlot* s = slotFor(name, UniformKind::kVec4)) {
    s->value[0] = color.r / 255.f;
    s->value[1] = color.g / 255.f;
    s->value[2] = color.b / 255.f;
    s->value[3] = color.a / 255.f;
  }
}

void
MaterialComponent::setTextureUniform(StringView name) {
  m_textureUniform = String(name);
}

void
MaterialComponent::apply(sf::RenderStates& states) const {
  if (!m_shaderAsset || !m_shaderAsset->isLoaded()) {
    return;
  }

  sf::Shader& shader = m_shaderAsset->shader();
  if (m_bindCurrentTexture) {
    // Bind the drawable's own texture (set on states at draw time) to the sampler.
    shader.setUniform(m_textureUniform, sf::Shader::CurrentTexture);
  }
  for (size_t i = 0; i < m_uniformCount; ++i) {
    const UniformSlot& u = m_uniforms[i];
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

} // namespace sfmx
