#include "scripts/RegisterAssetManager.h"

#include <functional>

#include "core/platform/Prerequisites.h"
#include "assets/AssetManager.h"

namespace sfmx
{

namespace script
{

void
registerAssetManager(sol::state_view lua) {
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

  lua["AssetManager"] = std::ref(AssetManager::instance());
}

}  // namespace script

}  // namespace sfmx