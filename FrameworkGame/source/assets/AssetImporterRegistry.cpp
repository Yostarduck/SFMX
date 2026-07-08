#include "assets/AssetImporterRegistry.h"

#include "assets/LuaAsset.h"
#include "assets/SoundAsset.h"
#include "assets/TextureAsset.h"

namespace sfmx
{

void
AssetImporterRegistry::registerBuiltins() {
  // Media chunks are tagged with their true byte encoding (never kRaw): the runtime
  // decoders dispatch on the tag, so it stays honest for tooling too.
  registerImporter<TextureAsset>(ChunkFormat::kPng,  ".png");
  registerImporter<TextureAsset>(ChunkFormat::kJpeg, ".jpg", ".jpeg");
  registerImporter<TextureAsset>(ChunkFormat::kBmp,  ".bmp");
  registerImporter<SoundAsset>(ChunkFormat::kOgg,  ".ogg");
  registerImporter<SoundAsset>(ChunkFormat::kWav,  ".wav");
  registerImporter<SoundAsset>(ChunkFormat::kFlac, ".flac");
  // Lua scripts are engine-native raw text (kRaw) -> LuaAsset. (Regression guard:
  // this rule was dropped when kImportRules became the registry; without it the
  // cooker skips `.lua`, the stale cooked script is rejected by the newer format
  // version, and every ScriptComponent silently fails to resolve.)
  registerImporter<LuaAsset>(ChunkFormat::kRaw, ".lua");
}

const ImportRule*
AssetImporterRegistry::findForExtension(StringView ext) const {
  const auto it = m_rules.find(String(ext));
  return it != m_rules.end() ? &it->second : nullptr;
}

} // namespace sfmx
