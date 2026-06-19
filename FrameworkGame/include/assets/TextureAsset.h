#pragma once

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
   */
  bool
  decodeFrom(AssetFileReader& reader);

 private:
  sf::Texture m_texture;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::TextureAsset)
