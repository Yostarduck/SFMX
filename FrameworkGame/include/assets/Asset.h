#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/AssetMetadata.h"
#include "utils/TypeTraits.h"
#include "utils/UUID.h"

namespace sfmx
{

/**
 * @brief Lifecycle state of an @ref IAsset.
 *
 * An asset starts @ref kUnloaded, is decoded into @ref kLoaded by its codec, or
 * left @ref kFailed if decoding could not produce a usable resource. @ref
 * kLoading is reserved for the async path (TODO); synchronous decode goes straight
 * from unloaded to loaded/failed.
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
 * An asset is created at *load time* (never in the game loop[not yet at least]) and owned through
 * an @c SPtr by the AssetManager's cache (TODO); it is not pooled. It carries its
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

 protected:
  FORCEINLINE void
  setMetadata(const AssetMetadata& meta) { m_metadata = meta; }

  FORCEINLINE void
  setState(AssetState state) { m_state = state; }

  AssetMetadata m_metadata;
  AssetState    m_state = AssetState::kUnloaded;
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
