#include <doctest/doctest.h>

#include <cstdio>

#include "core/platform/Prerequisites.h"
#include "core/MemoryDataStream.h"
#include "core/FileSystem.h"
#include "assets/AssetFile.h"

using namespace sfmx;

// Note: sfmx::UUID is qualified because on Windows <rpcdce.h> (via stduuid's
// system generator, pulled in through AssetMetadata.h) defines a global ::UUID.

TEST_CASE("AssetFile round-trip in memory") {
  AssetFileWriter writer;

  AssetMetadata meta;
  meta.uuid         = sfmx::UUID::createRandom();
  meta.assetType    = sfmx::UUID::createFromName("TextureAsset");
  meta.creationTime = 1234567u;
  meta.version      = 2u;
  std::snprintf(meta.typeName, sizeof(meta.typeName), "%s", "TextureAsset");
  std::snprintf(meta.name, sizeof(meta.name), "%s", "hero_sprite");
  std::snprintf(meta.sourcePath, sizeof(meta.sourcePath), "%s", "content/hero.webp");
  writer.setMetadata(meta);

  const sfmx::UUID dep0 = sfmx::UUID::createRandom();
  const sfmx::UUID dep1 = sfmx::UUID::createRandom();
  writer.addReference(dep0);
  writer.addReference(dep1);

  const Vector<uint8> payload = {0xDE, 0xAD, 0xBE, 0xEF, 1, 2, 3, 4, 5};
  CHECK(writer.addChunk(payload.data(), payload.size(), ChunkFormat::kWebP) == 0u);

  auto buffer = MakeShared<MemoryDataStream>();
  REQUIRE(writer.writeTo(*buffer));

  buffer->seek(0);
  AssetFileReader reader;
  REQUIRE(reader.open(buffer));

  CHECK(reader.metadata().uuid == meta.uuid);
  CHECK(reader.metadata().assetType == meta.assetType);
  CHECK(reader.metadata().creationTime == 1234567u);
  CHECK(reader.metadata().version == 2u);
  CHECK(String(reader.metadata().typeName) == "TextureAsset");
  CHECK(String(reader.metadata().name) == "hero_sprite");
  CHECK(String(reader.metadata().sourcePath) == "content/hero.webp");

  REQUIRE(reader.references().size() == 2u);
  CHECK(reader.references()[0] == dep0);
  CHECK(reader.references()[1] == dep1);

  REQUIRE(reader.chunkCount() == 1u);
  CHECK(reader.chunk(0).format == ChunkFormat::kWebP);
  CHECK(reader.chunk(0).compression == ChunkCompression::kNone);
  CHECK(reader.chunk(0).size == payload.size());

  Vector<uint8> readBack;
  REQUIRE(reader.readChunk(0, readBack));
  CHECK(readBack == payload);
}

TEST_CASE("AssetFile multiple chunks keep tags and offsets") {
  AssetFileWriter writer;
  writer.setMetadata(AssetMetadata{});  // all defaults

  const Vector<uint8> a(1000, 0xABu);   // highly compressible (zstd shrinks it)
  const Vector<uint8> b = {1, 2, 3};
  const Vector<uint8> empty;

  writer.addChunk(a.data(), a.size(), ChunkFormat::kRaw, ChunkCompression::kZstd);
  writer.addChunk(b.data(), b.size(), ChunkFormat::kOgg);
  writer.addChunk(empty.data(), empty.size());  // 0-byte chunk

  auto buffer = MakeShared<MemoryDataStream>();
  REQUIRE(writer.writeTo(*buffer));
  buffer->seek(0);

  AssetFileReader reader;
  REQUIRE(reader.open(buffer));
  REQUIRE(reader.chunkCount() == 3u);
  // Chunk 0 was actually zstd-compressed: tag kept and on-disk size < raw size.
  CHECK(reader.chunk(0).compression == ChunkCompression::kZstd);
  CHECK(reader.chunk(0).rawSize == a.size());
  CHECK(reader.chunk(0).size < reader.chunk(0).rawSize);
  CHECK(reader.chunk(1).format == ChunkFormat::kOgg);
  CHECK(reader.chunk(2).size == 0u);
  // Chunks are laid out back to back: chunk 1 starts right after chunk 0 (by the
  // on-disk/compressed size).
  CHECK(reader.chunk(1).offset == reader.chunk(0).offset + reader.chunk(0).size);

  Vector<uint8> r0;
  Vector<uint8> r1;
  Vector<uint8> r2;
  REQUIRE(reader.readChunk(0, r0));  // transparently inflated back to the original
  REQUIRE(reader.readChunk(1, r1));
  REQUIRE(reader.readChunk(2, r2));
  CHECK(r0 == a);
  CHECK(r1 == b);
  CHECK(r2.empty());
}

TEST_CASE("AssetFile zstd falls back to uncompressed when it would not shrink") {
  AssetFileWriter writer;
  writer.setMetadata(AssetMetadata{});

  // 3 bytes: zstd's frame overhead exceeds the input, so the writer stores it raw
  // and clears the tag (the no-grow fallback) rather than bloating the chunk.
  const Vector<uint8> tiny = {1, 2, 3};
  writer.addChunk(tiny.data(), tiny.size(), ChunkFormat::kRaw, ChunkCompression::kZstd);

  auto buffer = MakeShared<MemoryDataStream>();
  REQUIRE(writer.writeTo(*buffer));
  buffer->seek(0);

  AssetFileReader reader;
  REQUIRE(reader.open(buffer));
  REQUIRE(reader.chunkCount() == 1u);
  CHECK(reader.chunk(0).compression == ChunkCompression::kNone);  // downgraded
  CHECK(reader.chunk(0).size == tiny.size());
  CHECK(reader.chunk(0).rawSize == tiny.size());

  Vector<uint8> r0;
  REQUIRE(reader.readChunk(0, r0));
  CHECK(r0 == tiny);
}

TEST_CASE("AssetFile rejects a non-SFMX stream") {
  auto buffer = MakeShared<MemoryDataStream>();
  const Vector<uint8> garbage(64, 0x77u);
  buffer->write(garbage.data(), garbage.size());
  buffer->seek(0);

  AssetFileReader reader;
  CHECK_FALSE(reader.open(buffer));
  CHECK_FALSE(reader.isOpen());
}

TEST_CASE("AssetFile rejects a truncated file") {
  AssetFileWriter writer;
  writer.setMetadata(AssetMetadata{});
  const Vector<uint8> payload(256, 0x33u);
  writer.addChunk(payload.data(), payload.size(), ChunkFormat::kRaw);

  auto full = MakeShared<MemoryDataStream>();
  REQUIRE(writer.writeTo(*full));

  // Header is intact but the metadata/chunk it points to is past the end:
  // open() must fail cleanly (no huge alloc, no out-of-bounds read).
  const size_t truncated = 50;
  REQUIRE(full->size() > truncated);
  auto cut = MakeShared<MemoryDataStream>(full->data(), truncated);

  AssetFileReader reader;
  CHECK_FALSE(reader.open(cut));
}

TEST_CASE("AssetFile round-trip on disk") {
  const FileSystemPath path =
      FileSystem::tempDirectory() / "sfmx_assetfile_test.sfmxasset";
  FileSystem::remove(path);

  {
    AssetFileWriter writer;
    AssetMetadata meta;
    meta.uuid = sfmx::UUID::createRandom();
    std::snprintf(meta.name, sizeof(meta.name), "%s", "disk_asset");
    writer.setMetadata(meta);

    const Vector<uint8> payload(300, 0x5Au);
    writer.addChunk(payload.data(), payload.size(), ChunkFormat::kRaw);

    SPtr<DataStream> out = FileSystem::createAndOpenFile(path);
    REQUIRE(out != nullptr);
    REQUIRE(writer.writeTo(*out));
    out->close();
  }

  SPtr<DataStream> in = FileSystem::openFile(path, AccessMode::kRead);
  REQUIRE(in != nullptr);

  AssetFileReader reader;
  REQUIRE(reader.open(in));
  CHECK(String(reader.metadata().name) == "disk_asset");
  REQUIRE(reader.chunkCount() == 1u);

  Vector<uint8> back;
  REQUIRE(reader.readChunk(0, back));
  CHECK(back.size() == 300u);
  CHECK(back[0] == 0x5Au);

  // Release every handle to the file before deleting (Windows locks open files).
  reader.close();
  in.reset();
  CHECK(FileSystem::remove(path));
}
