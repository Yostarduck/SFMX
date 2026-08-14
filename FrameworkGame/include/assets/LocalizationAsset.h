#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/Asset.h"

namespace sfmx
{
class AssetFileReader;

class SFMX_UTILITY_EXPORT LocalizationAsset : public AssetT<LocalizationAsset>
{
public:

  NODISCARD FORCEINLINE const UnorderedMap<String, UnorderedMap<String, String>>&
  localizations() const { return m_localizations; }


  bool 
  decodeFrom(AssetFileReader& reader);

private:

  UnorderedMap<String, UnorderedMap<String, String>> m_localizations;

};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::LocalizationAsset)