#include "assets/AssetImporterRegistry.h"

#include "assets/MusicAsset.h"
#include "assets/LuaAsset.h"
#include "assets/ShaderAsset.h"
#include "assets/SoundAsset.h"
#include "assets/TextureAsset.h"
#include "assets/FontAsset.h"

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
  // Mp3 is ALWAYS music (never an sfx), so it cooks to MusicAsset by extension
  // regardless of folder. Ogg/wav/flac default to SoundAsset above; the cooker
  // promotes them to MusicAsset only under a `music/` folder (see AssetCooker).
  registerImporter<MusicAsset>(ChunkFormat::kMp3, ".mp3");
  // Lua scripts are engine-native raw text (kRaw) -> LuaAsset. (Regression guard:
  // this rule was dropped when kImportRules became the registry; without it the
  // cooker skips `.lua`, the stale cooked script is rejected by the newer format
  // version, and every ScriptComponent silently fails to resolve.)
  registerImporter<LuaAsset>(ChunkFormat::kRaw, ".lua");
  registerImporter<FontAsset>(ChunkFormat::kTtf,  ".ttf");
  registerImporter<FontAsset>(ChunkFormat::kOtf,  ".otf");
  // A loose `.frag` is a fragment-only shader (SFML supplies the default vertex
  // stage); it cooks as one stage-tagged chunk through the default path. A
  // `.shader` manifest lists several stage files, so it needs the cook hook to
  // emit one chunk per stage — the rule format is only a fallback tag there.
  registerImporter<ShaderAsset>(ShaderChunk::kFrag, ".frag");
  registerImporterCooked<ShaderAsset>(ChunkFormat::kRaw, &ShaderAsset::cookManifest,
                                      ".shader");
}

const ImportRule*
AssetImporterRegistry::findForExtension(StringView ext) const {
  const auto it = m_rules.find(String(ext));
  return it != m_rules.end() ? &it->second : nullptr;
}

} // namespace sfmx
