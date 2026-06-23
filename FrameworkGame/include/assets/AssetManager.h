#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/Asset.h"
#include "assets/AssetCodecRegistry.h"
#include "assets/AssetMetadata.h"
#include "utils/Module.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

class IAssetCodec;

/**
 * @brief Constrains @ref AssetManager::load / @ref AssetManager::get to types
 *        that are actually assets (derive from @ref IAsset).
 *
 * Turns a misuse like @c load<int> into a clear "constraint not satisfied" error
 * at the call site, instead of a deep failure inside @c static_pointer_cast.
 */
template<typename T>
concept AssetType = std::is_base_of_v<IAsset, T>;

/**
 * @brief Global asset registry + cache: turns a directory of `.sfmxasset` files
 *        into UUID-addressable, lazily-decoded, shared resources.
 *
 * Lifecycle (a @ref Module): @c startUp, then @c registerCodec for each asset
 * type, then @c mount one or more directories (cheap header scan → catalog),
 * then @c load by UUID. Decoding is dispatched by @c assetType through the owned
 * @ref AssetCodecRegistry; loaded assets are cached and shared via @c SPtr.
 *
 * Loading is synchronous in v1 (async is a later milestone). Assets are created
 * at load time and are not pooled.
 */
class SFMX_UTILITY_EXPORT AssetManager : public Module<AssetManager>
{
 public:
  /** @brief Register a codec (delegates to the owned registry). */
  void
  registerCodec(SPtr<IAssetCodec> codec);

  /**
   * @brief Recursively scan @p directory for `.sfmxasset` files and catalog them
   *        (UUID → path + metadata) via a cheap header read. Does NOT decode.
   * @return The number of assets cataloged from this directory.
   */
  size_t
  mount(const FileSystemPath& directory);

  /**
   * @brief Load (decode if needed) the asset @p id, caching the result.
   *
   * Resolves the asset's references (dependencies) first, so they are cached and
   * available. @return The shared asset, or @c nullptr if @p id is not cataloged,
   * its file cannot be opened, or no codec handles its type.
   */
  NODISCARD SPtr<IAsset>
  load(const UUID& id);

  /** @brief @ref load with a concrete type check; @c nullptr on type mismatch. */
  template<AssetType T>
  NODISCARD SPtr<T>
  load(const UUID& id);

  /** @brief The cached asset for @p id, or @c nullptr if not loaded (no decode). */
  NODISCARD SPtr<IAsset>
  get(const UUID& id) const;

  /** @brief @ref get with a concrete type check; @c nullptr on type mismatch. */
  template<AssetType T>
  NODISCARD SPtr<T>
  get(const UUID& id) const;

  /** @brief Whether @p id is known to the catalog (mounted). */
  NODISCARD bool
  isCataloged(const UUID& id) const;

  /** @brief Whether @p id is currently decoded and cached. */
  NODISCARD bool
  isLoaded(const UUID& id) const;

  /** @brief Cataloged metadata for @p id (cheap scan), or @c nullptr if unknown. */
  NODISCARD const AssetMetadata*
  metadataOf(const UUID& id) const;

  /** @brief Evict @p id from the cache (a caller's @c SPtr keeps it alive). */
  void
  unload(const UUID& id);

  /** @brief Evict every cached asset (the catalog is kept). */
  void
  unloadAll();

 protected:
  // Destroys cached assets while SFML is still alive (call shutDown before the
  // render window dies — same ordering rule as the other Modules).
  void
  onShutDown() override;

 private:
  friend class Module<AssetManager>;
  explicit AssetManager() = default;

  struct CatalogEntry {
    FileSystemPath path;
    AssetMetadata  metadata;
  };

  UnorderedMap<UUID, CatalogEntry>  m_catalog;
  UnorderedMap<UUID, SPtr<IAsset>>  m_cache;     // Loaded
  UnorderedSet<UUID>                m_loading;   // guards against reference cycles
  AssetCodecRegistry                m_codecs;
};

template<AssetType T>
SPtr<T>
AssetManager::load(const UUID& id) {
  SPtr<IAsset> asset = load(id);
  if (nullptr == asset) {
    return nullptr;
  }
  if (asset->typeId() != TypeTraits<T>::getTypeId()) {
    return nullptr;
  }
  return std::static_pointer_cast<T>(asset);
}

template<AssetType T>
SPtr<T>
AssetManager::get(const UUID& id) const {
  SPtr<IAsset> asset = get(id);
  if (nullptr == asset) {
    return nullptr;
  }
  if (asset->typeId() != TypeTraits<T>::getTypeId()) {
    return nullptr;
  }
  return std::static_pointer_cast<T>(asset);
}

} // namespace sfmx
