#include "assets/AssetImporterRegistry.h"

#include "assets/MusicAsset.h"
#include "assets/SoundAsset.h"
#include "assets/TextureAsset.h"

namespace sfmx
{

void
AssetImporterRegistry::registerBuiltins() {
  // Each chunk is tagged with its true byte encoding (never kRaw for media): the
  // runtime decoders dispatch on the tag, so it stays honest for tooling too.
  registerImporter<TextureAsset>(ChunkFormat::kPng,  ".png");
  registerImporter<TextureAsset>(ChunkFormat::kJpeg, ".jpg", ".jpeg");
  registerImporter<TextureAsset>(ChunkFormat::kBmp,  ".bmp");
  registerImporter<SoundAsset>(ChunkFormat::kOgg,  ".ogg");
  registerImporter<SoundAsset>(ChunkFormat::kWav,  ".wav");
  registerImporter<SoundAsset>(ChunkFormat::kFlac, ".flac");
  // Mp3 is ALWAYS music (never an sfx), so it cooks to MusicAsset by extension
  // regardless of folder. Ogg/wav/flac default to SoundAsset above; the cooker
  // promotes them to MusicAsset only under a `music/` folder (see AssetCooker).
  registerImporter<MusicAsset>(ChunkFormat::kMp3, ".mp3");
}

const ImportRule*
AssetImporterRegistry::findForExtension(StringView ext) const {
  const auto it = m_rules.find(String(ext));
  return it != m_rules.end() ? &it->second : nullptr;
}

} // namespace sfmx
