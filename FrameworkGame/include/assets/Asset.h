#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/AssetMetadata.h"
#include "utils/TypeTraits.h"
#include "utils/UUID.h"

namespace sfmx
{

class AssetFileReader;
class AssetManager;

/**
 * @brief Lifecycle state of an @ref IAsset.
 *
 * An asset starts @ref kUnloaded, is decoded into @ref kLoaded by its codec, or
 * left @ref kFailed if decoding could not produce a usable resource. @ref
 * kLoading marks an asset whose async decode is in flight (its worker-side
 * @ref IAsset::decodeCPU has not yet been finalized on the main thread);
 * synchronous decode goes straight from unloaded to loaded/failed.
 */
enum class AssetState : uint8 {
  kUnloaded = 0,
  kLoading,
  kLoaded,
  kFailed,
};

/**
 * @brief Base for every loadable runtime resource (texture, audio, mesh, ...).
 *
 * An asset is created at *load time* (not in the steady-state game loop) and owned
 * through an @c SPtr by the AssetManager's cache; it is not pooled. It carries its
 * @ref AssetMetadata (id, assetType, name, ...) and a @ref AssetState. The codec
 * for its @c assetType fills the concrete resource and flips the state.
 *
 * Prefer deriving from @ref AssetT, which supplies @ref typeId for you.
 */
class SFMX_UTILITY_EXPORT IAsset
{
 public:
  virtual ~IAsset() = default;

  /** @brief This asset's metadata (id, assetType, name, sourcePath, ...). */
  NODISCARD FORCEINLINE const AssetMetadata&
  metadata() const { return m_metadata; }

  /** @brief Current lifecycle state. */
  NODISCARD FORCEINLINE AssetState
  state() const { return m_state; }

  /** @brief True once the resource decoded successfully and is ready to use. */
  NODISCARD FORCEINLINE bool
  isLoaded() const { return AssetState::kLoaded == m_state; }

  /** @brief Concrete-type id; equals the @c assetType written to metadata. */
  NODISCARD virtual const UUID&
  typeId() const = 0;

  /**
   * @brief Decode this asset's payload from @p reader (synchronous, one shot).
   *
   * Each concrete asset implements this: read chunks, build the resource, stamp
   * metadata, and flip @ref state to @ref AssetState::kLoaded / @ref
   * AssetState::kFailed. This is the synchronous path (@ref AssetManager::load).
   * @return True on success.
   */
  virtual bool
  decodeFrom(AssetFileReader& reader) = 0;

  /**
   * @brief Async phase 1 — the thread-safe part of the decode (runs on a WORKER
   *        thread): file IO, decompression, and any CPU decode that needs no GPU
   *        context. It must NOT touch pools, the AssetManager, or GL resources.
   *
   * Default: run the whole @ref decodeFrom. That is correct for byte/PCM assets
   * (@c SoundAsset, @c MusicAsset, @c LuaAsset, @c FontAsset) whose decode is
   * main-thread-free. GPU-backed assets (@c TextureAsset) override this to decode
   * only to a CPU-side intermediate and defer the upload to @ref finalize.
   * @return True if the CPU-side decode succeeded.
   */
  virtual bool
  decodeCPU(AssetFileReader& reader) { return decodeFrom(reader); }

  /**
   * @brief Async phase 2 — the main-thread finalize (GPU upload, etc.), run by the
   *        AssetManager's per-frame pump after @ref decodeCPU completed on a worker.
   *
   * Default: nothing to do (the resource is already usable after @ref decodeCPU).
   * @c TextureAsset overrides this to upload its CPU image to the @c sf::Texture and
   * flip @ref state to @ref AssetState::kLoaded.
   * @return True once the asset is fully usable.
   */
  virtual bool
  finalize() { return true; }

 protected:
  FORCEINLINE void
  setMetadata(const AssetMetadata& meta) { m_metadata = meta; }

  FORCEINLINE void
  setState(AssetState state) { m_state = state; }

  AssetMetadata m_metadata;
  AssetState    m_state = AssetState::kUnloaded;

  // The AssetManager drives the async lifecycle (marks kLoading before handing the
  // asset to a worker, stamps catalog metadata) — it needs the protected setters.
  friend class AssetManager;
};

/**
 * @brief CRTP helper that supplies a concrete asset's @ref typeId from its
 *        @ref TypeTraits (which must be declared via @c DECLARE_TYPE_TRAITS).
 *
 * Derive as @c class TextureAsset : public AssetT<TextureAsset>.
 */
template<typename Derived>
class AssetT : public IAsset
{
 public:
  NODISCARD const UUID&
  typeId() const override { return TypeTraits<Derived>::getTypeId(); }
};

} // namespace sfmx
