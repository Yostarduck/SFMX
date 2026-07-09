#include "assets/AssetCooker.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>

#include "assets/AssetFile.h"
#include "assets/AssetImporterRegistry.h"
#include "assets/AssetMetadata.h"
#include "assets/LuaAsset.h"
#include "assets/MusicAsset.h"
#include "core/DataStream.h"
#include "core/FileSystem.h"
#include "utils/TypeTraits.h"
#include "utils/UUID.h"

namespace sfmx
{

/*
 * HOW TO ADD A NEW SOURCE FORMAT (the two independent seams)
 * ----------------------------------------------------------
 * A format touches two sides that never couple, because they run in different
 * places at different times:
 *
 *   IMPORT (this offline cooker): wraps the source bytes into a tagged chunk. It
 *          only copies bytes, so it never links the format's decode library. The
 *          extension -> (asset type, ChunkFormat) mapping is data in the
 *          @ref AssetImporterRegistry, not a table here.
 *   DECODE (the shipped runtime): turns those chunk bytes back into an asset,
 *          dispatching on the chunk's ChunkFormat tag. This is where a decode
 *          library (libwebp, ...) lives.
 *
 * A format MODULE (e.g. SFMX::ImageWebP) registers BOTH halves from its own init,
 * teaching the engine a new format with no core edit:
 *   - IMPORT: AssetImporterRegistry::registerImporter<TextureAsset>(kWebP, ".webp")
 *   - DECODE: AssetManager::registerDecoder<sf::Image>(kWebP, MakeShared<WebPImageDecoder>())
 * The engine-native formats SFML decodes directly (PNG/JPG/BMP, OGG/WAV/FLAC) are
 * the built-ins seeded by AssetImporterRegistry::registerBuiltins().
 *
 * A brand-new ASSET TYPE (e.g. `.voxel` -> VoxelAsset) adds a real decode codec:
 * register an IAssetCodec keyed by the new asset type id (AssetManager::registerCodec),
 * plus its import rule here — same two seams, decode side is a codec not a decoder.
 */

bool
AssetCooker::cookFile(const FileSystemPath& source,
                      const FileSystemPath& sourceRoot,
                      const FileSystemPath& outputDir) {
  // Lowercase the extension so the lookup is case-insensitive (.PNG == .png).
  String ext = source.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](ansichar c) {
    return static_cast<ansichar>(std::tolower(static_cast<unsigned char>(c)));
  });

  // The registry owns "extension -> (asset type, chunk format)". Modules register
  // their extensions into it, so this cooker needs no per-format knowledge.
  const ImportRule* rule = AssetImporterRegistry::instance().findForExtension(ext);
  if (nullptr == rule) {
    return false;  // unsupported extension -> skipped
  }

  // Path relative to the source root, with forward slashes -> a stable, portable
  // id source for createFromName (purely lexical, no filesystem access).
  const FileSystemPath rel = source.lexically_relative(sourceRoot);
  const String relStr = rel.generic_string();

  // Folder convention: anything under `music/` cooks as MusicAsset regardless of
  // extension, so streaming music is opt-in by LOCATION (mp3 is already music by
  // rule; ogg/wav/flac get promoted here). The chunk keeps its true byte-format tag
  // (kOgg/kMp3/...) — only the asset TYPE flips, since decode dispatches on type.
  bool underMusicDir = false;
  if (rel.begin() != rel.end()) {
    String first = rel.begin()->string();
    std::transform(first.begin(), first.end(), first.begin(), [](ansichar c) {
      return static_cast<ansichar>(std::tolower(static_cast<unsigned char>(c)));
    });
    underMusicDir = (first == "music");
  }

  ImportRule effective = *rule;
  if (underMusicDir && effective.assetType != TypeTraits<MusicAsset>::getTypeId()) {
    effective.assetType = TypeTraits<MusicAsset>::getTypeId();
    effective.typeName  = TypeTraits<MusicAsset>::getTypeName();
  }
  else if (ext == ".mp3" && !underMusicDir) {
    // mp3 is always music; finding one loose is almost certainly misplaced content.
    std::cerr << "AssetCooker: WARNING mp3 outside music/ (" << relStr
              << ") — cooked as MusicAsset; move it under resources/music/"
              << std::endl;
  }

  const Vector<uint8> bytes = FileSystem::fastRead(source);
  if (bytes.empty()) {
    std::cerr << "AssetCooker: could not read " << source.string() << std::endl;
    return false;
  }

  AssetMetadata meta;
  meta.uuid         = UUID::createFromName(relStr);
  meta.assetType    = effective.assetType;
  meta.creationTime = 0;  // deterministic: re-cooks are byte-identical
  std::snprintf(meta.typeName, sizeof(meta.typeName), "%s", effective.typeName);
  std::snprintf(meta.name, sizeof(meta.name), "%s", relStr.c_str());
  std::snprintf(meta.sourcePath, sizeof(meta.sourcePath), "%s", relStr.c_str());

  AssetFileWriter writer;
  writer.setMetadata(meta);
  writer.addChunk(bytes.data(), bytes.size(), effective.format);

  FileSystemPath out = outputDir / rel;
  out.replace_extension(".sfmxasset");

  SPtr<DataStream> os = FileSystem::createAndOpenFile(out);
  if (nullptr == os || !writer.writeTo(*os)) {
    std::cerr << "AssetCooker: could not write " << out.string() << std::endl;
    return false;
  }
  os->close();
  return true;
}

CookStats
AssetCooker::cookDirectory(const FileSystemPath& sourceDir,
                           const FileSystemPath& outputDir) {
  CookStats stats;
  if (!FileSystem::isDirectory(sourceDir)) {
    std::cerr << "AssetCooker: not a directory: " << sourceDir.string() << std::endl;
    return stats;
  }

  FileSystem::forEachFileChildRecursive(
      sourceDir, [&](const FileSystemPath& path) {
        if (!FileSystem::isFile(path)) {
          return;
        }
        if (cookFile(path, sourceDir, outputDir)) {
          ++stats.cooked;
        }
        else {
          ++stats.skipped;
        }
      });

  //TODO: Logger!
  std::cout << "AssetCooker: cooked " << stats.cooked << ", skipped "
            << stats.skipped << " (" << sourceDir.string() << " -> "
            << outputDir.string() << ")" << std::endl;
  return stats;
}

} // namespace sfmx
