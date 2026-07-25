#pragma once

#include <sol/sol.hpp>

namespace sfmx
{

namespace script
{

/**
 * @brief Bind the asset loading API to Lua: a minimal @c Asset handle usertype and a
 *        global @c Assets table with @c loadAsync(name, cb) / @c loadSync(name).
 *
 * @c Assets.loadAsync("foo.png", function(asset) ... end) kicks off a background load
 * and runs the callback (on the main thread, from the AssetManager pump) once the asset
 * is ready. @c Assets.loadSync("foo.png") blocks and returns the asset. Names are
 * resolved to UUIDs via @c UUID::createFromName, matching the cooker.
 */
void
registerAssetManager(sol::state_view lua);

}  // namespace script

}  // namespace sfmx
