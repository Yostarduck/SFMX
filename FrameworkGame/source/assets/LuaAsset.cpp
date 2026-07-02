#include "assets/LuaAsset.h"

#include "assets/AssetFile.h"

namespace sfmx
{

bool
LuaAsset::decodeFrom(AssetFileReader& reader) {
  setMetadata(reader.metadata());

  if (reader.chunkCount() == 0) {
    setState(AssetState::kFailed);
    return false;
  }

  // The chunk is the raw .lua source bytes; keep them as text for the
  // ScriptEngine to compile. Load-time only, so the copy is fine.
  Vector<uint8> bytes;
  if (!reader.readChunk(0, bytes)) {
    setState(AssetState::kFailed);
    return false;
  }

  m_script.assign(bytes.begin(), bytes.end());
  setState(AssetState::kLoaded);
  return true;
}

} // namespace sfmx
