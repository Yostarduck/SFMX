#pragma once

#include "assets/IDecoder.h"

namespace sf { class Image; }

namespace sfmx
{

/**
 * @brief Convenience alias: the image case of the generic @ref IDecoder seam.
 *
 * Image format modules (e.g. @c SFMX::ImageWebP) implement this and register via
 * @c AssetManager::registerDecoder<sf::Image>. It is just @c IDecoder<sf::Image> — a
 * future audio format would use @c IDecoder<sf::SoundBuffer> through the same registry,
 * with no new register/find pair in the core.
 */
using IImageDecoder = IDecoder<sf::Image>;

} // namespace sfmx
