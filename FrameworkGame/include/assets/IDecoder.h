#pragma once

#include "core/platform/Prerequisites.h"

namespace sfmx
{

/**
 * @brief Decodes encoded chunk bytes into an in-memory @c TOutput (e.g. an
 *        @c sf::Image for textures, an @c sf::SoundBuffer for audio).
 *
 * The generic seam that keeps format libraries (libwebp, ...) OUT of the core: the
 * core declares this interface and a registry (see @ref AssetManager::registerDecoder);
 * a format module implements @c IDecoder<TOutput> and registers an instance at startup
 * — the same runtime registration a DLL plugin would use later. The core never links a
 * format lib; it only holds pointers to this interface.
 *
 * ONE mechanism for every output domain: images register @c IDecoder<sf::Image>, a
 * future audio format registers @c IDecoder<sf::SoundBuffer>, etc. — no per-domain
 * register/find pair. Dispatch is by the chunk's @c ChunkFormat tag (written at cook
 * time): the decoder does NOT declare its own format (the tag is the registration key),
 * and there is no byte-sniffing.
 *
 * @tparam TOutput The in-memory type the encoded bytes decode into.
 */
template <typename TOutput>
class IDecoder
{
 public:
  virtual ~IDecoder() = default;

  /** @brief Decode @p size bytes at @p bytes into @p out. @return False on failure. */
  NODISCARD virtual bool
  decode(const uint8* bytes, size_t size, TOutput& out) const = 0;
};

} // namespace sfmx
