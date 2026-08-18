#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/IAssetCodec.h"
#include "assets/ShaderAsset.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief Codec that decodes shader stage chunks into a @ref ShaderAsset.
 *
 * Keyed by @c TypeTraits<ShaderAsset>::getTypeId(). Stateless — register one
 * instance with the @ref AssetCodecRegistry.
 */
class SFMX_UTILITY_EXPORT ShaderCodec : public IAssetCodec
{
 public:
  NODISCARD const UUID&
  assetType() const override { return TypeTraits<ShaderAsset>::getTypeId(); }

  NODISCARD SPtr<IAsset>
  create() const override { return MakeShared<ShaderAsset>(); }

  NODISCARD SPtr<IAsset>
  decode(AssetFileReader& reader) const override;
};

} // namespace sfmx
