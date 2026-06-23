#pragma once

#include <SFML/Audio/SoundBuffer.hpp>

#include "core/platform/Prerequisites.h"
#include "assets/Asset.h"

namespace sfmx
{

class AssetFileReader;

/**
 * @brief An audio sample asset: wraps an @c sf::SoundBuffer decoded from an
 *        audio chunk (Ogg/WAV/FLAC bytes) via SFML's built-in loader.
 *
 * The @c sf::SoundBuffer holds fully-decoded PCM (the "sound effect" path,
 * @c SourceComponent's kSound backend) — distinct from streaming @c sf::Music,
 * which is not an asset. Decoding is load-time and needs no audio device (it is
 * just PCM decode); only playback does.
 */
class SFMX_UTILITY_EXPORT SoundAsset : public AssetT<SoundAsset>
{
 public:
  NODISCARD FORCEINLINE const sf::SoundBuffer&
  buffer() const { return m_buffer; }

  /**
   * @brief Decode chunk 0 (audio bytes) into the buffer, stamping metadata and
   *        flipping @ref state to @ref AssetState::kLoaded / @ref
   *        AssetState::kFailed.
   * @return True on success. The codec calls this; not for the game loop.
   */
  bool
  decodeFrom(AssetFileReader& reader);

 private:
  sf::SoundBuffer m_buffer;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::SoundAsset)
