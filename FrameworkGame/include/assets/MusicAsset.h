#pragma once

#include "core/platform/Prerequisites.h"
#include "core/platform/STDHeaders.h"
#include "assets/Asset.h"

namespace sfmx
{

class AssetFileReader;

/**
 * @brief A streaming music asset: holds the ENCODED audio bytes (Ogg/Mp3/...)
 *        resident in memory but never decodes them — playback streams on demand.
 *
 * The non-resident-at-decode counterpart to @ref SoundAsset. A sound effect
 * decodes its whole PCM into an @c sf::SoundBuffer at load; music is long, so it
 * would blow the memory budget to decode. Instead this asset keeps only the small
 * encoded blob, and each @ref SourceComponent opens its OWN @c sf::Music over that
 * blob via @c openFromMemory (a live per-instance cursor — a stream is not shared).
 *
 * CRUCIAL: @c sf::Music::openFromMemory does NOT copy; it reads from the given
 * memory for the music's whole lifetime. So this asset (its @ref bytes) MUST outlive
 * every @c sf::Music opened over it — the SourceComponent keeps the @c SPtr alive.
 *
 * This buys music a UUID + catalog + cook (location-independent, cookable) while
 * keeping the streaming property. Because the encoded bytes ARE resident, the asset
 * uses the normal @ref AssetState::kLoaded state — no new "cataloged handle" state.
 */
class SFMX_UTILITY_EXPORT MusicAsset : public AssetT<MusicAsset>
{
 public:
  /** @brief The encoded audio bytes (Ogg/Mp3/...); the backing store for playback. */
  NODISCARD FORCEINLINE const Vector<uint8>&
  bytes() const { return m_encodedBytes; }

  /**
   * @brief Take chunk 0 (encoded audio bytes) as-is, stamping metadata and flipping
   *        @ref state to @ref AssetState::kLoaded / @ref AssetState::kFailed.
   *
   * Deliberately does NOT decode (no @c loadFromMemory) — that is the whole point of
   * a streaming asset. @return True on success. The codec calls this; not the loop.
   */
  bool
  decodeFrom(AssetFileReader& reader);

 private:
  Vector<uint8> m_encodedBytes;  //!< resident encoded blob, kept alive for sf::Music
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::MusicAsset)
