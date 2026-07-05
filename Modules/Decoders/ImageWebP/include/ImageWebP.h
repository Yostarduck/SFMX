#pragma once

namespace sfmx
{
namespace imagewebp
{

/**
 * @brief Register WebP support with the engine (decode + import rule).
 *
 * Call once at startup. Explicit (no static initializers) so it migrates cleanly to
 * a DLL plugin's load-time init later. Registers both seams, each guarded by the
 * target module being started, so the same call works in the runtime path (decoder
 * only, @c AssetManager up) and the cook path (import rule only, @c AssetImporterRegistry up):
 *   - DECODE: an @c IDecoder<sf::Image> for @c ChunkFormat::kWebP with the @c AssetManager,
 *             so a @c TextureAsset whose chunk is kWebP decodes via libwebp.
 *   - IMPORT: a @c ".webp" -> TextureAsset(kWebP) rule with the @c AssetImporterRegistry,
 *             so the cooker wraps `.webp` sources.
 * The core never links libwebp itself; only this module does.
 */
void
registerModule();

} // namespace imagewebp
} // namespace sfmx
