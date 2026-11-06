#pragma once
#include <SFML/Graphics/Shader.h>

#include "core/platform/Prerequisites.h"
#include "assets/Asset.h"


namespace sfmx
{

class AssetFileReader;

class SFMX_UTILITY_EXPORT MaterialAsset : public AssetT<MaterialAsset>
{
  public:
  NODISCARD FORCEINLINE const sf::Shader&
  shader() const { return m_shader; }

  NODISCARD FORCEINLINE sf::Shader&
  shader() { return m_shader; }

  bool
  decodeFrom(AssetFileReader& reader);

  private:
  sf::Shader m_shader;
  Vector<uint8> m_bytes;

};

}

