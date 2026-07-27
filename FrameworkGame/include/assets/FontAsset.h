#pragma once

#include <SFML/Graphics/Font.hpp>

#include "core/platform/Prerequisites.h"
#include "core/platform/STDHeaders.h"
#include "assets/Asset.h"

namespace sfmx
{

class AssetFileReader;

class SFMX_UTILITY_EXPORT FontAsset : public AssetT<FontAsset>
{
  public:
  NODISCARD FORCEINLINE const sf::Font&
  font() const { return m_font; }

  NODISCARD FORCEINLINE sf::Font&
  font() { return m_font; }

  bool
  decodeFrom(AssetFileReader& reader);

  private:
  sf::Font m_font;
  Vector<uint8> m_bytes;
};
} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::FontAsset)
