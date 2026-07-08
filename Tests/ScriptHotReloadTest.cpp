#include <doctest/doctest.h>

#include <cstdio>

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"
#include "core/FileSystem.h"
#include "core/MemoryDataStream.h"
#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include "assets/LuaAsset.h"
#include "assets/LuaCodec.h"
#include "utils/TypeTraits.h"

using namespace sfmx;

// Debug-only raw-script mode: LuaAsset takes its text from the source .lua (so edits
// are picked up without re-cooking) instead of the cooked chunk. This is the core of
// the F5 script hot-reload. Raw mode uses an ABSOLUTE rawScriptDir here so the test
// never touches the global content root.

namespace {

struct ManagerScope {
  ManagerScope() {
    if (AssetManager::isStarted()) {
      AssetManager::shutDown();
    }
    AssetManager::startUp();
  }
  ~ManagerScope() {
    if (AssetManager::isStarted()) {
      AssetManager::shutDown();
    }
  }
};

// Build an in-memory .sfmxasset for a LuaAsset: sourcePath names the raw file, the
// chunk holds the (stale) cooked text.
SPtr<MemoryDataStream>
makeCookedLua(const ansichar* sourcePath, const String& cookedText) {
  AssetFileWriter writer;
  AssetMetadata meta;
  meta.assetType = TypeTraits<LuaAsset>::getTypeId();
  std::snprintf(meta.sourcePath, sizeof(meta.sourcePath), "%s", sourcePath);
  writer.setMetadata(meta);
  writer.addChunk(cookedText.data(), cookedText.size(), ChunkFormat::kRaw);

  auto buffer = MakeShared<MemoryDataStream>();
  REQUIRE(writer.writeTo(*buffer));
  buffer->seek(0);
  return buffer;
}

String
decodeLuaText(const SPtr<MemoryDataStream>& buffer) {
  buffer->seek(0);
  AssetFileReader reader;
  REQUIRE(reader.open(buffer));
  LuaCodec codec;
  SPtr<IAsset> asset = codec.decode(reader);
  REQUIRE(asset != nullptr);
  return std::static_pointer_cast<LuaAsset>(asset)->script();
}

} // namespace

TEST_CASE("Raw-script mode reads the source .lua and picks up edits (debug hot-reload)") {
#if USING(SFMX_DEBUG_MODE)
  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_raw_lua";
  FileSystem::removeAll(dir);
  FileSystem::createDirectories(dir);

  auto writeSource = [&](const char* text) {
    SPtr<DataStream> s = FileSystem::createAndOpenFile(dir / "hero.lua");
    REQUIRE(s != nullptr);
    s->write(text, std::char_traits<char>::length(text));
    s->close();
  };

  const String cooked = "return function() return 'COOKED' end\n";
  const SPtr<MemoryDataStream> asset = makeCookedLua("hero.lua", cooked);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();

  // Raw mode ON with an ABSOLUTE source dir -> LuaAsset reads dir/hero.lua, not the chunk.
  writeSource("return function() return 'RAW v1' end\n");
  mgr.setRawScriptMode(true, dir.string());
  CHECK(mgr.getRawScriptMode());
  CHECK(decodeLuaText(asset) == "return function() return 'RAW v1' end\n");

  // Edit the source and decode again -> the new text is picked up (hot-reload core).
  writeSource("return function() return 'RAW v2' end\n");
  CHECK(decodeLuaText(asset) == "return function() return 'RAW v2' end\n");

  // Source missing -> clean fallback to the cooked chunk.
  mgr.setRawScriptMode(true, (dir / "nope").string());
  CHECK(decodeLuaText(asset) == cooked);

  // Raw mode OFF -> cooked chunk regardless of sources.
  mgr.setRawScriptMode(false, dir.string());
  CHECK_FALSE(mgr.getRawScriptMode());
  CHECK(decodeLuaText(asset) == cooked);

  FileSystem::removeAll(dir);
#else
  MESSAGE("raw-script mode is debug-only; skipped in this build");
#endif
}
