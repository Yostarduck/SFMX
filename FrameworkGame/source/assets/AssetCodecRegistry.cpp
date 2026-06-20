#include "assets/AssetCodecRegistry.h"

#include "assets/AssetFile.h"

namespace sfmx
{

void
AssetCodecRegistry::registerCodec(SPtr<IAssetCodec> codec) {
  if (nullptr == codec) {
    return;
  }
  // Read the key before sinking the SPtr (operator[] is evaluated before the
  // move-assign, so the codec is still alive when its assetType is copied in).
  // Last registration wins, so a mod can override a built-in codec on purpose.
  const UUID& assetType = codec->assetType();
  m_codecs[assetType] = std::move(codec);
}

const IAssetCodec*
AssetCodecRegistry::find(const UUID& assetType) const {
  const auto it = m_codecs.find(assetType);
  return it != m_codecs.end() ? it->second.get() : nullptr;
}

bool
AssetCodecRegistry::hasCodec(const UUID& assetType) const {
  return m_codecs.find(assetType) != m_codecs.end();
}

SPtr<IAsset>
AssetCodecRegistry::decode(AssetFileReader& reader) const {
  const UUID& assetType = reader.metadata().assetType;
  const IAssetCodec* codec = find(assetType);
  if (nullptr == codec) {
    std::cerr << "AssetCodecRegistry: no codec for assetType "
              << assetType.toString() << " (asset '" << reader.metadata().name
              << "')" << std::endl;
    return nullptr;
  }
  return codec->decode(reader);
}

} // namespace sfmx
