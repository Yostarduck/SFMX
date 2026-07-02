#include "assets/AssetCooker.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>

#include "assets/AssetFile.h"
#include "assets/AssetMetadata.h"
#include "assets/LuaAsset.h"
#include "assets/SoundAsset.h"
#include "assets/TextureAsset.h"
#include "core/DataStream.h"
#include "core/FileSystem.h"
#include "utils/TypeTraits.h"
#include "utils/UUID.h"

namespace sfmx
{

namespace {

/** @brief What a source extension cooks into (filled by @ref targetForExtension). */
struct CookTarget
{
  UUID            assetType;
  const ansichar* typeName;
  ChunkFormat     format;
};

/** @brief Headroom for rules with several extensions for one format (.jpg/.jpeg). */
constexpr size_t kMaxExtsPerRule = 4;

/**
 * @brief One import rule: which source extensions cook into which asset + format.
 *
 * @c typeId / @c typeName are pulled from @ref TypeTraits, never hand-typed, so
 * the cooked metadata cannot desync from the real asset type (the typo the table
 * exists to kill). The extensions are the only literals, declared once. See
 * @ref kImportRules below for the full walkthrough of adding a new format.
 */
struct ImportRule
{
  const UUID&     (*typeId)();
  const ansichar* (*typeName)();
  ChunkFormat     format;
  StringView      extensions[kMaxExtsPerRule];  // trailing empty entries = unused
};

/** @brief Build a rule, deriving id/name from @p TAsset's @ref TypeTraits. */
template <typename TAsset, typename... Exts>
constexpr ImportRule
makeRule(ChunkFormat format, Exts... exts) {
  return ImportRule{&TypeTraits<TAsset>::getTypeId,
                    &TypeTraits<TAsset>::getTypeName,
                    format,
                    {StringView(exts)...}};
}

/**
 * @brief Source of truth for "source extension -> what it cooks into". One row
 *        per (asset type, chunk format); `.mp3` (and anything not listed) is
 *        skipped. To support a new source format you add a row here, plus its
 *        decode counterpart in the runtime (walkthrough below).
 *
 * HOW TO ADD A NEW FORMAT (the two independent seams)
 * ---------------------------------------------------
 * A format always touches two sides that never couple, because they run in
 * different places at different times:
 *
 *   IMPORT (this offline cooker): wraps the source bytes into a tagged chunk.
 *                                 It only copies bytes, so it never links the
 *                                 format's decode library (libwebp, etc.).
 *   DECODE (the shipped runtime): turns those chunk bytes back into an asset.
 *                                 This is where the decode library lives.
 *
 * A mod shipped as a DLL registers BOTH halves from its own init, teaching the
 * engine a new format with no core recompile: the import row (here, or a future
 * registry, see bottom) and the decode codec (@ref AssetManager::registerCodec).
 *
 * Case 1: a new ENCODING of an existing asset type (e.g. WebP -> TextureAsset).
 *   No new asset type and no new codec, because TextureAsset already owns image
 *   decode. You only extend what it understands.
 *     IMPORT: make sure the chunk tag exists (@ref ChunkFormat in
 *             AssetMetadata.h) and add a row:
 *                 makeRule<TextureAsset>(ChunkFormat::kWebP, ".webp"),
 *     DECODE: teach TextureAsset::decodeFrom to handle the new tag. Today it just
 *             sniffs the bytes (SFML loadFromMemory autodetects PNG/JPG/BMP); WebP
 *             is the first format SFML cannot sniff, so the asset branches on the
 *             chunk tag:
 *                 if (reader.chunk(0).format == ChunkFormat::kWebP) {
 *                   // decode via libwebp -> RGBA -> sf::Texture. Only this branch
 *                   // links libwebp; per the design it lives in a module, not core.
 *                 }
 *                 else {
 *                   m_texture.loadFromMemory(bytes.data(), bytes.size());
 *                 }
 *
 * Case 2: a brand-new ASSET TYPE (e.g. a mod's `.voxel` -> VoxelAsset).
 *   Here the decode side is a real codec, keyed by the new asset type id.
 *     IMPORT: pick a chunk tag (kRaw is the escape hatch for opaque engine bytes)
 *             and add a row:
 *                 makeRule<VoxelAsset>(ChunkFormat::kRaw, ".voxel"),
 *     DECODE: implement IAssetCodec for VoxelAsset and register it at startup (or
 *             from the mod DLL's init), exactly like TextureCodec / SoundCodec:
 *                 class VoxelCodec : public IAssetCodec {
 *                   const UUID& assetType() const override {
 *                     return TypeTraits<VoxelAsset>::getTypeId();  // DECLARE_TYPE_TRAITS(VoxelAsset)
 *                   }
 *                   SPtr<IAsset> decode(AssetFileReader& reader) const override { ... }
 *                 };
 *                 AssetManager::instance().registerCodec(MakeShared<VoxelCodec>());
 *
 * Today this table is edited in source. When DLL/format modules land it gets
 * promoted to an AssetImporterRegistry (mirroring @ref AssetCodecRegistry): the
 * same module that calls registerCodec also registers its ImportRule, so neither
 * half needs a core edit. Iterating a static table vs a registered map is the
 * same loop, so that promotion is mechanical.
 */
const ImportRule kImportRules[] = {
  // Each chunk is tagged with its true byte encoding (never kRaw for media): the
  // decoders sniff today, but the tag stays honest for tooling / future dispatch.
  makeRule<TextureAsset>(ChunkFormat::kPng,  ".png"),
  makeRule<TextureAsset>(ChunkFormat::kJpeg, ".jpg", ".jpeg"),
  makeRule<TextureAsset>(ChunkFormat::kBmp,  ".bmp"),
  makeRule<SoundAsset>(ChunkFormat::kOgg,  ".ogg"),
  makeRule<SoundAsset>(ChunkFormat::kWav,  ".wav"),
  makeRule<SoundAsset>(ChunkFormat::kFlac, ".flac"),
  // Lua scripts cook as opaque text bytes (kRaw); LuaAsset keeps them as a String.
  makeRule<LuaAsset>(ChunkFormat::kRaw, ".lua"),
};

/** @brief Map a lowercase extension (".png", ...) to its cook target.
 *  @return True if supported; @p out is filled. Unsupported extensions
 *          (including ".mp3") return false. */
bool
targetForExtension(StringView ext, CookTarget& out) {
  for (const ImportRule& rule : kImportRules) {
    for (const StringView& candidate : rule.extensions) {
      if (candidate.empty()) {
        break;  // trailing unused slots -> done with this rule
      }
      if (candidate == ext) {
        out = {rule.typeId(), rule.typeName(), rule.format};
        return true;
      }
    }
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
    return false;  // unsupported extension -> skipped
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
