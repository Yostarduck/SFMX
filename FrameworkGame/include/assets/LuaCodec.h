#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/IAssetCodec.h"
#include "assets/LuaAsset.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief Codec that decodes a raw chunk into a @ref LuaAsset (script text).
 *
 * Keyed by @c TypeTraits<LuaAsset>::getTypeId(). Stateless — register one
 * instance with the @ref AssetCodecRegistry.
 */
class SFMX_UTILITY_EXPORT LuaCodec : public IAssetCodec
{
 public:
  NODISCARD const UUID&
  assetType() const override { return TypeTraits<LuaAsset>::getTypeId(); }

  NODISCARD SPtr<IAsset>
  decode(AssetFileReader& reader) const override;
};

} // namespace sfmx
