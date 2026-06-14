#pragma once

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Rect.hpp>

#include "core/platform/Prerequisites.h"

namespace sfmx::Atlas
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

/**
 * @brief Divide the atlas into a uniform @p columns x @p rows grid.
 *
 * All cells are the same size (atlasSize / columns / rows). Partial tiles
 * at the right/bottom edge are discarded.
 *
 * @param atlasSize Width and height of the sprite sheet in pixels.
 * @param columns   Number of columns to split into.
 * @param rows      Number of rows to split into.
 * @return @p columns x @p rows rectangles covering the full atlas.
 */
NODISCARD Vector<sf::IntRect> getSpriteRectsByGrid(
    const sf::Vector2u& atlasSize,
    uint32 columns,
    uint32 rows);

/**
 * @brief Divide the atlas into fixed-size cells.
 *
 * Tiles are placed left-to-right, top-to-bottom. A partial tile at the
 * right or bottom edge is included and clipped to the atlas bounds.
 *
 * @param atlasSize    Width and height of the sprite sheet in pixels.
 * @param spriteWidth  Width of each sprite in pixels.
 * @param spriteHeight Height of each sprite in pixels.
 * @return All full and (where applicable) clipped partial tiles.
 */
NODISCARD Vector<sf::IntRect> getSpriteRectsByRectSize(
    const sf::Vector2u& atlasSize,
    uint32 spriteWidth,
    uint32 spriteHeight);

} // namespace sfmx::Atlas
