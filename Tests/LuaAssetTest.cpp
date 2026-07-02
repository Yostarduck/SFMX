#include <doctest/doctest.h>

#include <memory>

#include "core/platform/Prerequisites.h"
#include "core/MemoryDataStream.h"
#include "assets/AssetFile.h"
#include "assets/LuaAsset.h"
#include "assets/LuaCodec.h"
#include "utils/TypeTraits.h"

using namespace sfmx;

// A .lua script cooked as a raw chunk decodes back to its exact source text.

TEST_CASE("LuaAsset decodes the script text from a raw chunk") {
  const String source = "return function(self, dt)\n  self:transform()\nend\n";

  AssetFileWriter writer;
  AssetMetadata meta;
  meta.assetType = TypeTraits<LuaAsset>::getTypeId();
  writer.setMetadata(meta);
  writer.addChunk(source.data(), source.size(), ChunkFormat::kRaw);

  auto buffer = MakeShared<MemoryDataStream>();
  REQUIRE(writer.writeTo(*buffer));
  buffer->seek(0);

  AssetFileReader reader;
  REQUIRE(reader.open(buffer));

  LuaCodec codec;
  SPtr<IAsset> asset = codec.decode(reader);
  REQUIRE(asset != nullptr);
  CHECK(asset->isLoaded());
  CHECK(asset->typeId() == TypeTraits<LuaAsset>::getTypeId());

  SPtr<LuaAsset> lua = std::static_pointer_cast<LuaAsset>(asset);
  CHECK(lua->script() == source);
}

TEST_CASE("LuaAsset fails cleanly on a chunk-less container") {
  AssetFileWriter writer;
  writer.setMetadata(AssetMetadata{});  // no chunks

  auto buffer = MakeShared<MemoryDataStream>();
  REQUIRE(writer.writeTo(*buffer));
  buffer->seek(0);

  AssetFileReader reader;
  REQUIRE(reader.open(buffer));

  LuaCodec codec;
  SPtr<IAsset> asset = codec.decode(reader);
  REQUIRE(asset != nullptr);
  CHECK_FALSE(asset->isLoaded());  // 0 chunks → kFailed
}
