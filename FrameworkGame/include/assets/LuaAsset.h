#pragma once

#include "core/platform/Prerequisites.h"
#include "assets/Asset.h"

namespace sfmx
{

class AssetFileReader;

/**
 * @brief A Lua script asset: holds the script's source text, decoded from a raw
 *        chunk (the @c .lua bytes the cooker wrapped).
 *
 * Resident and immutable once loaded (a "catalog" asset like TextureAsset /
 * SoundAsset, referenced by UUID) — the @ref ScriptEngine compiles it from this
 * text. Decoding is load-time and needs no Lua VM; only running the script does.
 */
class SFMX_UTILITY_EXPORT LuaAsset : public AssetT<LuaAsset>
{
 public:
  /** @brief The script's source text (empty until decoded). */
  NODISCARD FORCEINLINE const String&
  script() const { return m_script; }

  /**
   * @brief Decode chunk 0 (the @c .lua source bytes) into the script text,
   *        stamping metadata and flipping @ref state to @ref AssetState::kLoaded
   *        / @ref AssetState::kFailed.
   * @return True on success. The codec calls this; not for the game loop.
   */
  bool
  decodeFrom(AssetFileReader& reader);

 private:
  String m_script;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::LuaAsset)
