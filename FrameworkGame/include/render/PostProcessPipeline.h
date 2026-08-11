#pragma once

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "core/platform/Prerequisites.h"

namespace sfmx
{

class SceneManager;
class ShaderAsset;

/**
 * @brief Full-screen post-processing: renders the scene to an offscreen target and
 *        runs it through a chain of fragment shaders (vignette, grade, CRT, ...).
 *
 * Per-window render state, not a Module — construct it in @c main next to the
 * window and destroy it before the window (its @c sf::RenderTexture holds GL
 * resources). The two ping-pong targets and the blit sprite are created once in
 * @ref init; @ref render allocates nothing per frame — uniform names are stored
 * once and passed by reference, and the fullscreen blit reuses a single sprite and
 * a stack @c sf::RenderStates.
 */
class SFMX_UTILITY_EXPORT PostProcessPipeline
{
 public:
  /** @brief Create the two offscreen targets at @p size. Returns false if the GPU
   *         could not allocate them (post-processing then stays disabled). */
  bool
  init(sf::Vector2u size);

  /** @brief Recreate the offscreen targets at a new @p size (call on window resize). */
  void
  onResize(sf::Vector2u size);

  /** @brief Append a fragment-shader pass. Setup-time; keeps the asset alive. */
  void
  addPass(SPtr<ShaderAsset> shader);

  /** @brief Whether any passes are registered (else @ref render draws straight through). */
  NODISCARD FORCEINLINE bool
  hasPasses() const { return !m_passes.empty(); }

  /**
   * @brief Draw @p scenes to @p window through the pass chain.
   *
   * With no passes (or uninitialised), draws the scene straight to @p window — the
   * caller's clear still applies. Otherwise the scene goes to an offscreen target
   * and each pass blits it forward; the last pass targets @p window with its default
   * view. @p timeSeconds feeds the @c u_time uniform.
   */
  void
  render(SceneManager& scenes, sf::RenderTarget& window, float timeSeconds);

 private:
  struct Pass
  {
    SPtr<ShaderAsset> shader;
  };

  sf::RenderTexture    m_bufferA;
  sf::RenderTexture    m_bufferB;
  Optional<sf::Sprite> m_fullscreen;      // built once; SFML3 sprite needs a texture
  Vector<Pass>         m_passes;
  sf::Vector2u         m_size;
  bool                 m_ready = false;

  // Uniform names bound once at construction, so render() never builds a std::string.
  const String m_uTexture    = "u_texture";
  const String m_uResolution = "u_resolution";
  const String m_uTime       = "u_time";
};

} // namespace sfmx
