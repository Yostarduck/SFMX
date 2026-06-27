#include "scripts/RegisterKeyboard.h"

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
    
    "load", [](AssetManager& a, UUID id) -> void { a.load(id); },
    "get", [](AssetManager& a, UUID id) -> void { a.get(id); },
    "isLoaded", [](AssetManager& a, UUID id) -> void { a.isLoaded(id); },
    "unload", [](AssetManager& a, UUID id) -> void { a.unload(id); },
    "unloadAll", [](AssetManager& a, UUID id) -> void { a.unloadAll(id); }
  );

  lua["AssetManager"] = std::ref(AssetManager::instance());
}

}  // namespace script

}  // namespace sfmx