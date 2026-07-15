#pragma once

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "core/platform/Prerequisites.h"
#include "assets/Asset.h"

namespace sfmx
{

class AssetFileReader;

/**
 * @brief A GPU texture asset: wraps an @c sf::Texture decoded from an image
 *        chunk (PNG/JPG bytes) via SFML's built-in image loader.
 *
 * The @c sf::Texture is held by value; its construction is a load-time GPU
 * upload (fine - outside the game loop). Decoding the chunk needs a valid OpenGL
 * context, which SFML provides through its internal shared context.
 */
class SFMX_UTILITY_EXPORT TextureAsset : public AssetT<TextureAsset>
{
 public:
  NODISCARD FORCEINLINE const sf::Texture&
  texture() const { return m_texture; }

  NODISCARD FORCEINLINE sf::Texture&
  texture() { return m_texture; }

  /**
   * @brief Decode chunk 0 (image bytes) into the texture, stamping metadata and
   *        flipping @ref state to @ref AssetState::kLoaded / @ref
   *        AssetState::kFailed.
   * @return True on success. The codec calls this; not for the game loop.
   *
   * Synchronous path = @ref decodeCPU followed by @ref finalize, both on the caller's
   * thread (the current, GL-owning thread).
   */
  bool
  decodeFrom(AssetFileReader& reader) override;

  /**
   * @brief Async phase 1 (WORKER thread): decode the image bytes into a CPU-side
   *        @c sf::Image only — no GPU/GL context touched. Leaves @ref state at
   *        @ref AssetState::kLoading on success, @ref AssetState::kFailed otherwise.
   */
  bool
  decodeCPU(AssetFileReader& reader) override;

  /**
   * @brief Async phase 2 (MAIN thread): upload the decoded @c sf::Image to the
   *        @c sf::Texture (needs the GL context) and flip @ref state to
   *        @ref AssetState::kLoaded / @ref AssetState::kFailed. Releases the CPU image.
   */
  bool
  finalize() override;

 private:
  sf::Texture m_texture;
  // CPU-side decode target for the async path; holds pixels only between decodeCPU
  // (worker) and finalize (main), then is released. Empty at rest.
  sf::Image   m_pendingImage;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::TextureAsset)
