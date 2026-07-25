#include "scripts/RegisterAssetManager.h"

#include <iostream>
#include <functional>

#include "core/platform/Prerequisites.h"
#include "assets/Asset.h"
#include "assets/AssetManager.h"
#include "utils/UUID.h"

namespace sfmx
{

namespace script
{

void
registerAssetManager(sol::state_view lua) {
  // The IAsset handle usertype itself is registered by registerIAsset (called just
  // before this). Here we only add the loading entry points.

  // A free-function table rather than a bound singleton: the instance is fetched
  // lazily at call time and guarded by isStarted(), so registration order stays safe.
  sol::table assets = lua.create_named_table("Assets");

  assets.set_function("loadAsync",
    [](const std::string& name, sol::protected_function cb) {
      if (!AssetManager::isStarted()) {
        return;
      }
      const UUID id = UUID::createFromName(name);
      if (cb.valid()) {
        // The callback holds a Lua closure; it fires later from the main-thread pump.
        // AssetManager::cancelAsyncLoads() releases any that never fired, before Lua dies.
        AssetManager::instance().loadAsync(id, [cb](SPtr<IAsset> asset) {
          sol::protected_function_result r = cb(asset);
          if (!r.valid()) {
            const sol::error err = r;
            std::cerr << "Assets.loadAsync callback error: " << err.what() << std::endl;
          }
        });
      }
      else {
        AssetManager::instance().loadAsync(id);
      }
    });

  assets.set_function("loadSync",
    [](const std::string& name) -> SPtr<IAsset> {
      if (!AssetManager::isStarted()) {
        return nullptr;
      }
      return AssetManager::instance().loadSync(UUID::createFromName(name));
    });

  // Full AssetManager surface (from origin/main): the started singleton exposed as a
  // global for scripts that resolve/inspect assets by UUID directly.
  lua.new_usertype<AssetManager>("AssetManager",
    sol::no_constructor,

    "load", [](AssetManager& a, const UUID& id) { return a.load(id); },
    "get", [](AssetManager& a, const UUID& id) { return a.get(id); },
    "isLoaded", [](AssetManager& a, const UUID& id) { return a.isLoaded(id); },
    "isCataloged",
    [](AssetManager& a, const UUID& id) { return a.isCataloged(id); },
    "metadataOf",
    [](AssetManager& a, const UUID& id) { return a.metadataOf(id); },
    "unload", [](AssetManager& a, const UUID& id) { a.unload(id); },
    "unloadAll", [](AssetManager& a) { a.unloadAll(); }
  );

  // Only bind the singleton global if the module is up: bindings are registered from
  // ScriptEngine::onStartUp, which may run before AssetManager::startUp (e.g. in tests).
  // Accessing instance() before startUp throws, so guard it.
  if (AssetManager::isStarted()) {
    lua["AssetManager"] = std::ref(AssetManager::instance());
  }
}

}  // namespace script

}  // namespace sfmx
