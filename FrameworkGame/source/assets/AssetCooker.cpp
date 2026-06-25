#include "assets/AssetCooker.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>

#include "assets/AssetFile.h"
#include "assets/AssetMetadata.h"
#include "assets/SoundAsset.h"
#include "assets/TextureAsset.h"
#include "core/DataStream.h"
#include "core/FileSystem.h"
#include "utils/TypeTraits.h"
#include "utils/UUID.h"

namespace sfmx
{

namespace {

/** @brief What a source extension cooks into. */
struct CookTarget
{
  UUID         assetType;
  const char*  typeName;
  ChunkFormat  format;
};

/** @brief Map a lowercase extension (".png", ...) to its cook target.
 *  @return True if supported; @p out is filled. Unsupported extensions
 *          (including ".mp3") return false. */
bool
targetForExtension(const String& ext, CookTarget& out) {
  if (ext == ".png") {
    out = {TypeTraits<TextureAsset>::getTypeId(), "TextureAsset", ChunkFormat::kPng};
    return true;
  }
  if (ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
    // No per-format tag for these; SFML's loadFromMemory auto-detects the bytes.
    out = {TypeTraits<TextureAsset>::getTypeId(), "TextureAsset", ChunkFormat::kRaw};
    return true;
  }
  if (ext == ".ogg") {
    out = {TypeTraits<SoundAsset>::getTypeId(), "SoundAsset", ChunkFormat::kOgg};
    return true;
  }
  if (ext == ".wav" || ext == ".flac") {
    out = {TypeTraits<SoundAsset>::getTypeId(), "SoundAsset", ChunkFormat::kRaw};
    return true;
  }
  return false;
}

} // namespace

bool
AssetCooker::cookFile(const FileSystemPath& source,
                      const FileSystemPath& sourceRoot,
                      const FileSystemPath& outputDir) {
  // Lowercase the extension so the lookup is case-insensitive (.PNG == .png).
  String ext = source.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](ansichar c) {
    return static_cast<ansichar>(std::tolower(static_cast<unsigned char>(c)));
  });

  CookTarget target;
  if (!targetForExtension(ext, target)) {
    return false;  // unsupported extension → skipped
  }

  // Path relative to the source root, with forward slashes -> a stable, portable
  // id source for createFromName (purely lexical, no filesystem access).
  const FileSystemPath rel = source.lexically_relative(sourceRoot);
  const String relStr = rel.generic_string();

  const Vector<uint8> bytes = FileSystem::fastRead(source);
  if (bytes.empty()) {
    std::cerr << "AssetCooker: could not read " << source.string() << std::endl;
    return false;
  }

  AssetMetadata meta;
  meta.uuid         = UUID::createFromName(relStr);
  meta.assetType    = target.assetType;
  meta.creationTime = 0;  // deterministic: re-cooks are byte-identical
  std::snprintf(meta.typeName, sizeof(meta.typeName), "%s", target.typeName);
  std::snprintf(meta.name, sizeof(meta.name), "%s", relStr.c_str());
  std::snprintf(meta.sourcePath, sizeof(meta.sourcePath), "%s", relStr.c_str());

  AssetFileWriter writer;
  writer.setMetadata(meta);
  writer.addChunk(bytes.data(), bytes.size(), target.format);

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
