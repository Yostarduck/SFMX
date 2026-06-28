#include "scripts/RegisterIAsset.h"

#include "core/platform/Prerequisites.h"
#include "assets/Asset.h"
#include "assets/AssetMetadata.h"

namespace sfmx
{

namespace script
{

void
registerIAsset(sol::state_view lua) {
  lua.new_enum<AssetState>("AssetState", {
    { "Unloaded", AssetState::kUnloaded },
    { "Loading",  AssetState::kLoading },
    { "Loaded",   AssetState::kLoaded },
    { "Failed",   AssetState::kFailed }
  });

  // Cataloged descriptor returned by IAsset::metadata(); read-only from Lua
  // since it mirrors the cached, on-disk header.
  lua.new_usertype<AssetMetadata>("AssetMetadata",
    sol::no_constructor,

    "uuid",         sol::readonly(&AssetMetadata::uuid),
    "assetType",    sol::readonly(&AssetMetadata::assetType),
    "creationTime", sol::readonly(&AssetMetadata::creationTime),
    "version",      sol::readonly(&AssetMetadata::version),

    "typeName",
    sol::readonly_property(
      [](const AssetMetadata& m) { return String(m.typeName); }),
    "name",
    sol::readonly_property(
      [](const AssetMetadata& m) { return String(m.name); }),
    "sourcePath",
    sol::readonly_property(
      [](const AssetMetadata& m) { return String(m.sourcePath); })
  );

  // Abstract base for every loadable resource; instances reach Lua through
  // AssetManager:load/get, never constructed from script.
  lua.new_usertype<IAsset>("IAsset",
    sol::no_constructor,

    "metadata", &IAsset::metadata,
    "state",    &IAsset::state,
    "isLoaded", &IAsset::isLoaded,
    "typeId",   &IAsset::typeId
  );
}

}  // namespace script

}  // namespace sfmx
