#include "assets/LuaAsset.h"

#include <cstdio>

#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include "core/FileSystem.h"

namespace sfmx
{

bool
LuaAsset::decodeFrom(AssetFileReader& reader) {
  setMetadata(reader.metadata());

#if USING(SFMX_DEBUG_MODE)
  // Dev hot-reload: in raw-script mode, take the script text from its source `.lua`
  // (resolved under the content root as <rawScriptDir>/<sourcePath>) instead of the
  // cooked chunk, so edits are picked up without re-cooking. Falls through to the
  // cooked chunk if the source isn't found (e.g. content root has no sources).
  if (AssetManager::isStarted() && AssetManager::instance().getRawScriptMode()) {
    const FileSystemPath source = FileSystem::resolve(
        FileSystemPath(AssetManager::instance().getRawScriptDir()) / metadata().sourcePath);
    const Vector<uint8> rawBytes = FileSystem::fastRead(source);
    if (!rawBytes.empty()) {
      m_script.assign(rawBytes.begin(), rawBytes.end());
      setState(AssetState::kLoaded);
      return true;
    }
    fprintf(stderr, "[Script] raw source not found (%s); using cooked chunk\n",
            source.string().c_str());
  }
#endif

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
