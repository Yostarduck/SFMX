#include "render/PostProcessPipeline.h"

#include <utility>

#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Shader.hpp>

#include "assets/ShaderAsset.h"
#include "scene/SceneManager.h"

namespace sfmx
{

namespace
{
constexpr sf::Color kSceneClear(24, 24, 28);
} // namespace

bool
PostProcessPipeline::init(sf::Vector2u size) {
  m_size = size;

  // GL targets, created once. SFML 3 uses resize() (no create); a false return means
  // the GPU refused the allocation, so post-processing simply stays off.
  if (!m_bufferA.resize(size) || !m_bufferB.resize(size)) {
    m_ready = false;
    return false;
  }

  m_bufferA.setSmooth(true);
  m_bufferB.setSmooth(true);

  // The single blit sprite reused by every pass; it needs a texture at construction.
  m_fullscreen.emplace(m_bufferA.getTexture());

  m_ready = true;
  return true;
}

void
PostProcessPipeline::onResize(sf::Vector2u size) {
  if (!m_ready) {
    return;
  }
  m_size = size;
  m_bufferA.resize(size);
  m_bufferB.resize(size);
}

void
PostProcessPipeline::addPass(SPtr<ShaderAsset> shader) {
  m_passes.push_back(Pass{std::move(shader)});
}

void
PostProcessPipeline::render(SceneManager& scenes,
                            sf::RenderTarget& window,
                            float timeSeconds) {
  // No chain (or GPU targets unavailable): draw the scene straight to the window.
  if (!m_ready || m_passes.empty()) {
    scenes.draw(window);
    return;
  }

  // 1) Scene -> first offscreen target.
  m_bufferA.clear(kSceneClear);
  scenes.draw(m_bufferA);
  m_bufferA.display();

  // 2) Ping-pong the image through each pass. Everything below is stack-only.
  sf::RenderTexture* src = &m_bufferA;
  sf::RenderTexture* dst = &m_bufferB;

  const sf::Glsl::Vec2 resolution(static_cast<float>(m_size.x),
                                  static_cast<float>(m_size.y));
  const size_t passCount = m_passes.size();

  for (size_t i = 0; i < passCount; ++i) {
    const bool last = (i + 1 == passCount);

    const ShaderAsset* asset = m_passes[i].shader.get();
    sf::Shader* shader =
        (nullptr != asset && asset->isLoaded())
            ? &const_cast<ShaderAsset*>(asset)->shader()
            : nullptr;

    sf::RenderStates states;
    states.shader = shader;
    if (nullptr != shader) {
      // CurrentTexture resolves at draw time to states.texture — which the sprite
      // sets to its own (the source) texture. Names passed by reference: no alloc.
      shader->setUniform(m_uTexture, sf::Shader::CurrentTexture);
      shader->setUniform(m_uResolution, resolution);
      shader->setUniform(m_uTime, timeSeconds);
    }

    // Point the reused sprite at the current source image (resets its rect to full).
    m_fullscreen->setTexture(src->getTexture(), true);

    if (last) {
      // Final pass writes to the window; its default view maps the sprite 1:1.
      window.setView(window.getDefaultView());
      window.draw(*m_fullscreen, states);
    }
    else {
      dst->clear();
      dst->draw(*m_fullscreen, states);
      dst->display();
      std::swap(src, dst);
    }
  }
}

} // namespace sfmx
