#pragma once

#include "core/platform/Prerequisites.h"

#include "assets/AssetMetadata.h"   // ChunkFormat
#include "utils/Module.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

class AssetFileWriter;

/**
 * @brief Optional cook hook: fills a writer's chunks from a source file itself,
 *        replacing the default "wrap the whole file as one chunk" behaviour.
 *
 * Used by multi-chunk sources (e.g. a shader manifest that pulls in several stage
 * files). Receives the source path, the source root, and the writer to append
 * chunks to. Returns false to skip the file (nothing usable cooked). Null on a
 * rule means the default single-chunk path — so existing formats are unchanged.
 */
using ChunkCookFn = bool (*)(const FileSystemPath& source,
                             const FileSystemPath& sourceRoot,
                             AssetFileWriter& writer);

/**
 * @brief What a source extension cooks into: the target asset type + chunk tag.
 *
 * @c assetType / @c typeName are pulled from @ref TypeTraits (never hand-typed) by
 * @ref AssetImporterRegistry::registerImporter, so cooked metadata cannot desync
 * from the real asset type. @c format is the @ref ChunkFormat the source bytes are
 * tagged with (the runtime dispatches decode on it — see @ref IImageDecoder).
 * @c cook, when set, takes over chunk production entirely (multi-chunk sources).
 */
struct ImportRule
{
  UUID            assetType;
  const ansichar* typeName = "Unknown";
  ChunkFormatId   format   = ChunkFormat::kRaw;
  ChunkCookFn     cook     = nullptr;
};

/**
 * @brief Extension -> @ref ImportRule registry driving the offline @ref AssetCooker.
 *
 * The extensibility seam for the IMPORT side, mirroring @ref AssetCodecRegistry on
 * the decode side: a format module teaches the engine a new source extension by 
 * calling @ref registerImporter from its own init — no core edit. WebP does 
 * exactly this (its `.webp` rule lives in @c SFMX::ImageWebP, not here), which is 
 * why @ref registerBuiltins covers only the engine-native formats SFML/SFMX decode 
 * directly (PNG/JPG/BMP, OGG/WAV/FLAC).
 *
 * A @ref Module because it is cross-cutting global state the cooker consults; only
 * the cook path starts it (the runtime game loop never cooks). It stores plain
 * data rules, not polymorphic importers: SFMX import only wraps bytes into a tagged
 * chunk (it never decodes at import), so there is no per-format import body to hold.
 */
class SFMX_UTILITY_EXPORT AssetImporterRegistry : public Module<AssetImporterRegistry>
{
 public:
  /**
   * @brief Register (or override) the rule for one or more source extensions.
   *
   * @tparam TAsset The asset type the extensions cook into; its id/name come from
   *         @ref TypeTraits<TAsset> (@c DECLARE_TYPE_TRAITS), never hand-typed.
   * @param format The @ref ChunkFormat to tag the wrapped bytes with.
   * @param extensions Lowercase, dot-prefixed extensions (".png", ".jpg", ...).
   */
  template <typename TAsset, typename... Exts>
  void
  registerImporter(ChunkFormatId format, Exts... extensions) {
    const ImportRule rule{TypeTraits<TAsset>::getTypeId(),
                          TypeTraits<TAsset>::getTypeName(),
                          format};
    const StringView exts[] = {StringView(extensions)...};
    for (const StringView& e : exts) {
      m_rules[String(e)] = rule;
    }
  }

  /**
   * @brief Like @ref registerImporter, but with a @ref ChunkCookFn that produces
   *        the chunks itself (multi-chunk sources).
   *
   * @param format The @ref ChunkFormat recorded on the rule; the cook hook is free
   *        to tag its own chunks per stage, so this is only a fallback tag.
   * @param cook The hook the cooker calls instead of the default single-chunk wrap.
   */
  template <typename TAsset, typename... Exts>
  void
  registerImporterCooked(ChunkFormatId format, ChunkCookFn cook, Exts... extensions) {
    const ImportRule rule{TypeTraits<TAsset>::getTypeId(),
                          TypeTraits<TAsset>::getTypeName(),
                          format,
                          cook};
    const StringView exts[] = {StringView(extensions)...};
    for (const StringView& e : exts) {
      m_rules[String(e)] = rule;
    }
  }

  /**
   * @brief Register the engine's built-in source formats (PNG/JPG/JPEG/BMP ->
   *        TextureAsset, OGG/WAV/FLAC -> SoundAsset). Call once after @c startUp.
   *
   * Module-provided formats (e.g. `.webp`) are NOT here — each module registers its
   * own extension from @c registerModule, so adding a format is a module, not a core edit.
   */
  void
  registerBuiltins();

  /**
   * @brief The rule for a lowercase, dot-prefixed extension (".png"), or @c nullptr
   *        if the extension is unsupported (the cooker then skips the file).
   */
  NODISCARD const ImportRule*
  findForExtension(StringView ext) const;

 private:
  friend class Module<AssetImporterRegistry>;
  explicit AssetImporterRegistry() = default;

  UnorderedMap<String, ImportRule> m_rules;  // extension (".png") -> rule
};

} // namespace sfmx
