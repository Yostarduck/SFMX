#include "gfx/InstanceDrawer.h"

#include <algorithm>
#include <iostream>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexBuffer.hpp>

#include "assets/AssetManager.h"
#include "assets/ShaderAsset.h"
#include "gfx/GfxRenderer.h"
#include "utils/UUID.h"

namespace sfmx
{

// Source-relative name the cooker mints the atlas shader's UUID from; the game
// ships Game/resources/shaders/instancedSprite.shader cooked under assets/.
static const ansichar* kAtlasShaderName = "shaders/instancedSprite.shader";

void
InstanceDrawer::onStartUp() {
  // Reserved generously so registering atlases never reallocates in practice;
  // handles are indices, so even a reallocation would not invalidate them.
  m_batches.reserve(32);
}

void
InstanceDrawer::onShutDown() {
  // Release the GPU buffers and the shader while the GL context is still alive.
  m_batches.clear();
  m_shader.reset();
  m_shaderResolved = false;
}

void
InstanceDrawer::setShader(SPtr<ShaderAsset> shader) {
  m_shader = std::move(shader);
  m_shaderResolved = true;
}

void
InstanceDrawer::resolveShader() {
  if (m_shaderResolved) {
    return;
  }

  if (AssetManager::isStarted()) {
    m_shader = AssetManager::instance().load<ShaderAsset>(
        UUID::createFromName(String(kAtlasShaderName)));
    m_shaderResolved = true;
  }
}

AtlasHandle
InstanceDrawer::registerAtlas(const sf::Texture* texture,
                              const sf::BlendMode& blend,
                              const Vector<sf::IntRect>& frames,
                              const sf::Vector2u& textureSize,
                              size_t reserveInstances) {
  for (uint32 i = 0; i < m_batches.size(); ++i) {
    if (m_batches[i].texture == texture && m_batches[i].blend == blend) {
      return AtlasHandle{i};
    }
  }

  AtlasBatch batch;
  batch.texture = texture;
  batch.blend = blend;
  batch.capacity = reserveInstances;
  batch.instances.reserve(reserveInstances);
  batch.custom.reserve(reserveInstances);

  const float texW = (textureSize.x > 0u) ? static_cast<float>(textureSize.x) : 1.f;
  const float texH = (textureSize.y > 0u) ? static_cast<float>(textureSize.y) : 1.f;
  const uint32 count =
      std::min(static_cast<uint32>(frames.size()), kMaxAtlasFrames);
  for (uint32 i = 0; i < count; ++i) {
    const sf::IntRect& r = frames[i];
    batch.frames[i] = sf::Glsl::Vec4(static_cast<float>(r.position.x) / texW,
                                     static_cast<float>(r.position.y) / texH,
                                     static_cast<float>(r.size.x) / texW,
                                     static_cast<float>(r.size.y) / texH);
  }
  batch.frameCount = count;

  m_batches.push_back(std::move(batch));
  return AtlasHandle{static_cast<uint32>(m_batches.size() - 1)};
}

void
InstanceDrawer::submit(const AtlasHandle& handle,
                       const sf::Vector2f& center,
                       uint32 frameIndex,
                       const sf::Vector2f& size,
                       float rotation,
                       const sf::Color& tint) {
  if (!handle.isValid() || handle.index >= m_batches.size()) {
    return;
  }

  AtlasBatch& batch = m_batches[handle.index];
  // Never grow past the reserved capacity: a reallocation here would be a
  // per-frame heap allocation on the draw path. Drop the overflow instead.
  if (batch.instances.size() >= batch.capacity) {
    return;
  }

  batch.instances.push_back(
      gfx::makeQuadInstance(center, tint, rotation, 0.f));

  gfx::QuadCustomData data;
  data.id = static_cast<int32>((frameIndex < batch.frameCount) ? frameIndex : 0u);
  data.x = size.x;
  data.y = size.y;
  data.z = 0.f;
  batch.custom.push_back(data);
}

void
InstanceDrawer::beginPass() {
  m_lastDrawCalls = 0u;
  for (AtlasBatch& batch : m_batches) {
    batch.instances.clear();
    batch.custom.clear();
  }
}

void
InstanceDrawer::flush(sf::RenderTarget& target) {
  if (!GfxRenderer::hasQuadRenderer()) {
    return;
  }

  resolveShader();
  if (nullptr == m_shader || !m_shader->isLoaded()) {
    if (!m_warnedNoShader) {
      m_warnedNoShader = true;
      std::cerr << "InstanceDrawer: " << kAtlasShaderName
                << " missing; instanced sprites will not draw\n";
    }
    return;
  }

  gfx::InstancedQuadRenderer* renderer = GfxRenderer::instance().quadRenderer();
  sf::Shader& shader = m_shader->shader();

  for (AtlasBatch& batch : m_batches) {
    const size_t count = batch.instances.size();
    if (0 == count) {
      continue;
    }

    if (!batch.buffer) {
      batch.buffer = MakeUnique<sf::VertexBuffer>();
      if (!batch.buffer->create(batch.capacity)) {
        batch.buffer.reset();
        continue;
      }
      batch.buffer->setUsage(sf::VertexBuffer::Usage::Stream);
      batch.buffer->setPrimitiveType(sf::PrimitiveType::Points);
    }

    if (!batch.buffer->update(batch.instances.data(), count, 0u)) {
      continue;
    }

    // The frame table is per-atlas but the program is shared, so set it before
    // each batch's draw; the renderer sets u_mvp/size/useTexture itself.
    shader.setUniformArray("u_frames", batch.frames.data(), batch.frameCount);

    gfx::QuadBatch quads;
    quads.instances = batch.buffer.get();
    quads.instanceCount = count;
    quads.mvp = target.getView().getTransform();
    quads.texture = batch.texture;
    quads.blendMode = batch.blend;
    // Per-instance size travels through custom data (x, y), so the batch-wide
    // size uniforms are unused by the atlas shader.
    quads.sizeAtBlend0 = {0.f, 0.f};
    quads.sizeAtBlend1 = {0.f, 0.f};
    quads.shader = &shader;
    quads.customData = batch.custom.data();
    quads.customDataCount = count;

    if (!renderer->draw(target, quads)) {
      if (!m_warnedUnsupported) {
        m_warnedUnsupported = true;
        std::cerr << "InstanceDrawer: driver does not support instancing; "
                     "instanced sprites will not draw\n";
      }
      return;
    }

    // Per-instance custom data forces a GL draw every kMaxCustomDataInstances,
    // so a big batch is several draws; report the real count for profiling.
    m_lastDrawCalls += static_cast<uint32>(
        (count + gfx::kMaxCustomDataInstances - 1) / gfx::kMaxCustomDataInstances);
  }
}

} // namespace sfmx
