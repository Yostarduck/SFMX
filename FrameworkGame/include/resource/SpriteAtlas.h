#pragma once

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Rect.hpp>

#include "core/platform/Prerequisites.h"

namespace sfmx
{

/**
 * @brief Auto-detect sprite rectangles from a texture atlas by scanning for
 *        connected non-transparent regions.
 *
 * Each connected (4-directional) region of opaque pixels is treated as one
 * sprite and its bounding box is returned. Results are sorted in reading
 * order (top-to-bottom, left-to-right).
 *
 * @param image          The source atlas image.
 * @param alphaThreshold Alpha values strictly greater than this are
 *                       considered opaque (0–255, default 1).
 * @param padding        Extra pixels added to each side of every rect
 *                       (for sprite bleeding, default 0).
 * @return Bounding rectangles of all detected sprites.
 */
NODISCARD Vector<sf::IntRect> detectSpriteRects(
    const sf::Image& image,
    uint8 alphaThreshold = 1,
    int32 padding = 0);

} // namespace sfmx
