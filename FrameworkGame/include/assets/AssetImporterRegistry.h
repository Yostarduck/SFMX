#pragma once

#include "core/platform/Prerequisites.h"

#include "assets/AssetMetadata.h"   // ChunkFormat
#include "utils/Module.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief What a source extension cooks into: the target asset type + chunk tag.
 *
 * @c assetType / @c typeName are pulled from @ref TypeTraits (never hand-typed) by
 * @ref AssetImporterRegistry::registerImporter, so cooked metadata cannot desync
 * from the real asset type. @c format is the @ref ChunkFormat the source bytes are
 * tagged with (the runtime dispatches decode on it — see @ref IImageDecoder).
 */
struct ImportRule
{
  UUID            assetType;
  const ansichar* typeName = "Unknown";
  ChunkFormatId   format   = ChunkFormat::kRaw;
};

/**
 * @brief Extension -> @ref ImportRule registry driving the offline @ref AssetCooker.
 *
 * The extensibility seam for the IMPORT side, mirroring @ref AssetCodecRegistry on
 * the decode side (and modelled on Chimera's AssetCodecManager): a format module
 * teaches the engine a new source extension by calling @ref registerImporter from
 * its own init — no core edit. WebP does exactly this (its `.webp` rule lives in
 * @c SFMX::ImageWebP, not here), which is why @ref registerBuiltins covers only the
 * engine-native formats SFML/SFMX decode directly (PNG/JPG/BMP, OGG/WAV/FLAC).
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
