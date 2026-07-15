#pragma once

#include <condition_variable>
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

  /** @brief Explicit synchronous alias of @ref load (reads + decodes now, blocking). */
  NODISCARD FORCEINLINE SPtr<IAsset>
  loadSync(const UUID& id) { return load(id); }

  /**
   * @brief Kick off a background decode of @p id, invoking @p onLoaded once it is
   *        ready (or failed). Returns immediately.
   *
   * The worker thread does the IO + CPU decode; the main-thread @ref finalize pump
   * completes any GPU upload, caches the asset, and fires @p onLoaded with it (check
   * @c asset->state() for success). @p onLoaded always fires from the pump on the
   * main thread — even on a cache hit — so it is safe to create nodes / touch pools
   * from it, and the timing is uniform. Multiple @c loadAsync calls for the same @p id
   * coalesce into one decode; every callback fires. The returned handle is the
   * (possibly still @ref AssetState::kLoading) asset; ignoring it is fine.
   *
   * @return The shared asset (already cached, in flight, or freshly created), or
   *         @c nullptr if @p id is not cataloged or has no codec.
   */
  SPtr<IAsset>
  loadAsync(const UUID& id, Function<void(SPtr<IAsset>)> onLoaded = {});

  /**
   * @brief Per-frame pump: finalize assets whose background decode completed and fire
   *        their @ref loadAsync callbacks. Call once per frame on the main thread.
   *
   * Bounded and allocation-free when idle. This is where @c kLoading → @c kLoaded
   * (GPU upload) happens, so components created this frame see freshly loaded assets.
   */
  void
  finalize();

  /**
   * @brief Join the worker and drop all pending async loads and their callbacks.
   *
   * Call this once when tearing down, BEFORE any subsystem a callback might reference
   * (e.g. the Lua state) is shut down — @ref onShutDown runs late in the Module order,
   * so lingering Lua-closure callbacks would otherwise be destroyed after Lua is gone.
   * Idempotent; @ref onShutDown also calls it.
   */
  void
  cancelAsyncLoads();

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
  // Spawns the async-load worker thread (idle until the first loadAsync).
  void
  onStartUp() override;

  // Joins the worker, then destroys cached assets while SFML is still alive (call
  // shutDown before the render window dies — same ordering rule as the other Modules).
  void
  onShutDown() override;

 private:
  friend class Module<AssetManager>;
  explicit AssetManager() = default;

  struct CatalogEntry {
    FileSystemPath path;
    AssetMetadata  metadata;
  };

  // -- Async loading (worker thread + main-thread finalize pump) --------------------
  // A load in flight: one job on the worker, N callbacks waiting on the main thread.
  struct LoadJob {
    UUID           id;
    SPtr<IAsset>   asset;   // shared with m_inflight; only one thread touches it at a time
    FileSystemPath path;
  };
  struct PendingLoad {
    SPtr<IAsset>                         asset;
    Vector<Function<void(SPtr<IAsset>)>> callbacks;
  };
  // A cache-hit callback deferred to the next pump (uniform timing, no reentrancy).
  struct ReadyCallback {
    SPtr<IAsset>                 asset;
    Function<void(SPtr<IAsset>)> callback;
  };

  // Worker body: pops jobs, does IO + decodeCPU, pushes ids to the done queue.
  void
  workerLoop();

  // Main thread: finalize one completed asset, cache it, fire its callbacks.
  void
  finalizeCompleted(const UUID& id);

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

  // Async loading. The worker owns nothing shared except the two queues (mutex-guarded);
  // m_inflight / m_readyCallbacks / m_cache are touched only on the main thread, so no
  // lock is needed on the manager's maps. An asset is handed worker↔main via the queues,
  // so exactly one thread accesses a given asset object at a time.
  Thread                            m_worker;
  Mutex                             m_jobMutex;
  std::condition_variable           m_jobCv;
  Deque<LoadJob>                    m_jobQueue;        // main → worker
  Mutex                             m_doneMutex;
  Deque<UUID>                       m_doneQueue;       // worker → main
  Atomic<bool>                      m_workerStop{false};
  UnorderedMap<UUID, PendingLoad>   m_inflight;        // main-thread only
  Vector<ReadyCallback>             m_readyCallbacks;  // main-thread only (cache-hit deferrals)
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
