#pragma once

#include <typeindex>

#include "core/platform/Prerequisites.h"
#include "assets/Asset.h"
#include "assets/AssetCodecRegistry.h"
#include "assets/AssetMetadata.h"
#include "assets/IDecoder.h"
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
   * @brief Register a decoder that turns @p format bytes into a @c TOutput.
   *
   * The one generic seam for format modules (e.g. @c SFMX::ImageWebP): the module
   * registers a decoder at startup so an asset can decode that format without the core
   * linking the format's library. ONE mechanism for every output domain — images use
   * @c registerDecoder<sf::Image>, a future audio format @c registerDecoder<sf::SoundBuffer>,
   * with no per-domain register/find pair. The @ref ChunkFormat tag is the key given here
   * (the decoder does not declare it), matched against the chunk's tag at load. Last
   * registration for a (@c TOutput, @c format) pair wins.
   */
  template <typename TOutput>
  void
  registerDecoder(ChunkFormatId format, SPtr<IDecoder<TOutput>> decoder) {
    if (nullptr == decoder) {
      return;
    }
    SPtr<IDecoderTable>& slot = m_decoders[std::type_index(typeid(TOutput))];
    if (nullptr == slot) {
      slot = MakeShared<DecoderTable<TOutput>>();
    }
    static_cast<DecoderTable<TOutput>*>(slot.get())->byFormat[format] =
        std::move(decoder);
  }

  /** @brief The @c TOutput decoder for @p format, or @c nullptr if none is registered. */
  template <typename TOutput>
  NODISCARD const IDecoder<TOutput>*
  findDecoder(ChunkFormatId format) const {
    const auto table = m_decoders.find(std::type_index(typeid(TOutput)));
    if (table == m_decoders.end()) {
      return nullptr;
    }
    const auto& byFormat =
        static_cast<const DecoderTable<TOutput>*>(table->second.get())->byFormat;
    const auto it = byFormat.find(format);
    return it != byFormat.end() ? it->second.get() : nullptr;
  }

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

  /**
   * @brief Re-decode @p id from its current on-disk state, replacing the cache entry.
   *
   * Evicts then re-loads, so the fresh asset reflects whatever the file (or, in debug
   * raw-script mode, the source `.lua`) currently holds. Callers holding the old
   * @c SPtr keep it until they re-resolve @p id. Used by dev hot-reload; reusable for
   * any asset type. @return the freshly decoded asset, or @c nullptr.
   */
  NODISCARD SPtr<IAsset>
  reload(const UUID& id);

#if USING(SFMX_DEBUG_MODE)
  // -- Debug: raw script mode (dev hot-reload) --------------------------------------
  // Debug-only surface: the whole trio is compiled out of release, so raw script
  // loading cannot be enabled there (it is not a silent no-op — it does not exist).
  /**
   * @brief Load Lua scripts from their raw source file instead of the cooked chunk.
   * @param enabled   Turn raw script loading on.
   * @param sourceDir Content-root-relative dir the sources live under (e.g. "resources").
   */
  void
  setRawScriptMode(bool enabled, StringView sourceDir);

  /** @brief Whether raw script loading is on (false unless set; debug builds only). */
  NODISCARD FORCEINLINE bool
  getRawScriptMode() const { return m_rawScripts; }

  /** @brief Content-root-relative dir raw script sources are read from. */
  NODISCARD FORCEINLINE const String&
  getRawScriptDir() const { return m_rawScriptDir; }
#endif // USING(SFMX_DEBUG_MODE)

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

  // Type-erased per-output-type decoder tables: one DecoderTable<TOutput> per output
  // type (sf::Image, sf::SoundBuffer, ...), each mapping ChunkFormat -> decoder. This
  // is what makes registerDecoder<T>/findDecoder<T> generic with no per-domain member.
  struct IDecoderTable {
    virtual ~IDecoderTable() = default;
  };
  template <typename TOutput>
  struct DecoderTable : IDecoderTable {
    UnorderedMap<ChunkFormatId, SPtr<IDecoder<TOutput>>> byFormat;
  };

  UnorderedMap<UUID, CatalogEntry>  m_catalog;
  UnorderedMap<UUID, SPtr<IAsset>>  m_cache;     // Loaded
  UnorderedSet<UUID>                m_loading;   // guards against reference cycles
  AssetCodecRegistry                m_codecs;
#if USING(SFMX_DEBUG_MODE)
  bool                              m_rawScripts   = false;       // debug-only dev flag
  String                            m_rawScriptDir = "resources"; // raw source dir (content-root-relative)
#endif
  // Keyed by the output C++ type; the inner table is keyed by ChunkFormat.
  UnorderedMap<std::type_index, SPtr<IDecoderTable>> m_decoders;
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
