#include "scripts/RegisterAssetManager.h"

#include <iostream>

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
  // Minimal asset handle: enough for a callback to receive a loaded asset and hand it
  // to a component (e.g. sprite:setTextureAsset(asset)). No constructor from Lua.
  lua.new_usertype<IAsset>("Asset",
    sol::no_constructor,
    "isLoaded", &IAsset::isLoaded);

  // A free-function table rather than a bound singleton: the AssetManager module is
  // NOT started yet when bindings are registered (ScriptEngine starts before it), so
  // the instance is fetched lazily at call time, guarded by isStarted().
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
}

}  // namespace script

}  // namespace sfmx
