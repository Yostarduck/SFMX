#pragma once

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"
#include "gfx/InstancedQuadRenderer.h"
#include "utils/Module.h"

namespace sf
{
class RenderTarget;
class Texture;
class VertexBuffer;
} // namespace sf

namespace sfmx
{

class ShaderAsset;

/** @brief Most atlas frames a single batch can index from the shader. */
constexpr uint32 kMaxAtlasFrames = 128u;

/** @brief Marks an @ref AtlasHandle that points at no batch. */
constexpr uint32 kInvalidAtlas = 0xFFFFFFFFu;

/**
 * @brief Opaque reference to one batch bucket inside the @ref InstanceDrawer.
 *
 * It is a plain index, not a pointer, so it stays valid even if the drawer's
 * bucket storage reallocates as more atlases register.
 */
struct AtlasHandle
{
  uint32 index = kInvalidAtlas;

  NODISCARD FORCEINLINE bool
  isValid() const { return kInvalidAtlas != index; }
};

/**
 * @brief Collects every sprite that shares an atlas and draws them in one
 *        instanced GPU call per atlas.
 *
 * A component that wants to be batched registers its atlas once
 * (@ref registerAtlas) and then, each frame from its own draw, hands the drawer
 * one instance (@ref submit). The scene traversal brackets each camera pass with
 * @ref beginPass (before) and @ref flush (after): submit fills per-atlas buffers
 * during the walk, flush uploads them and issues the batched draws.
 *
 * A @ref Module singleton, torn down before @ref GfxRenderer / the window so the
 * GPU buffers it owns die inside the GL context. Atlas sub-rects need a custom
 * program (the built-in quad shader samples the whole texture); the drawer
 * resolves it by name on first flush, or the game can set it with @ref setShader.
 */
class InstanceDrawer : public Module<InstanceDrawer>
{
 public:
  /**
   * @brief Register (or look up) the batch bucket for @p texture + @p blend.
   *
   * @param texture          The shared atlas texture; instances key off it.
   * @param blend            Blend mode for the whole batch.
   * @param frames           Sub-rects in pixels; frame index i selects frames[i].
   * @param textureSize      Atlas size in pixels, to normalise the frame rects.
   * @param reserveInstances Buffer capacity for this atlas, reserved once here.
   * @return A handle to submit against. Re-registering the same texture+blend
   *         returns the existing bucket (frames/capacity from the first call).
   */
  AtlasHandle
  registerAtlas(const sf::Texture* texture,
                const sf::BlendMode& blend,
                const Vector<sf::IntRect>& frames,
                const sf::Vector2u& textureSize,
                size_t reserveInstances);

  /** @brief Queue one quad for this frame's batch. Cheap; no allocation. */
  void
  submit(const AtlasHandle& handle,
         const sf::Vector2f& center,
         uint32 frameIndex,
         const sf::Vector2f& size,
         float rotation,
         const sf::Color& tint);

  /** @brief Reset every bucket to empty, keeping its capacity. Per camera pass. */
  void
  beginPass();

  /** @brief Upload and draw every non-empty bucket onto @p target. */
  void
  flush(sf::RenderTarget& target);

  /** @brief Override the atlas shader instead of resolving it by name. */
  void
  setShader(SPtr<ShaderAsset> shader);

  /** @brief Instanced GL draws issued by the last @ref flush (counts the
   *         1024-instance splits), for profiling. */
  NODISCARD FORCEINLINE uint32
  getLastDrawCalls() const { return m_lastDrawCalls; }

 protected:
  void onStartUp() override;
  void onShutDown() override;

 private:
  friend class Module<InstanceDrawer>;
  InstanceDrawer() = default;

  /** @brief Load the atlas shader by name once, if the game did not set one. */
  void
  resolveShader();

  /** @brief One atlas's persistent per-frame instance storage. */
  struct AtlasBatch
  {
    const sf::Texture*          texture = nullptr;
    sf::BlendMode               blend;
    size_t                      capacity = 0;
    Vector<gfx::QuadInstance>   instances;
    Vector<gfx::QuadCustomData> custom;
    UniquePtr<sf::VertexBuffer> buffer;
    Array<sf::Glsl::Vec4, kMaxAtlasFrames> frames;
    uint32                      frameCount = 0;
  };

  Vector<AtlasBatch> m_batches;
  SPtr<ShaderAsset>  m_shader;
  bool               m_shaderResolved   = false;
  bool               m_warnedNoShader   = false;
  bool               m_warnedUnsupported = false;
  uint32             m_lastDrawCalls    = 0u;
};

} // namespace sfmx
