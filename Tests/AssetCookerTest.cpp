#include <doctest/doctest.h>

#include <array>

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/SoundChannel.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"
#include "core/FileSystem.h"
#include "assets/AssetCooker.h"
#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include "assets/AssetMetadata.h"
#include "assets/SoundAsset.h"
#include "assets/SoundCodec.h"
#include "assets/TextureAsset.h"
#include "assets/TextureCodec.h"
#include "utils/TypeTraits.h"

using namespace sfmx;

// sfmx::UUID is qualified because on Windows <rpcdce.h> (via stduuid) defines a
// global ::UUID. M6a: the cooker wraps external media into .sfmxasset containers
// with a deterministic UUID derived from the path relative to the source root.

namespace {

// RAII: clean startUp/shutDown of the AssetManager even if a REQUIRE throws.
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

// Fresh, empty source/output pair under the temp directory.
struct CookDirs {
  FileSystemPath root;
  FileSystemPath src;
  FileSystemPath out;
  explicit CookDirs(const ansichar* name) {
    root = FileSystem::tempDirectory() / name;
    src  = root / "src";
    out  = root / "out";
    FileSystem::removeAll(root);
    FileSystem::createDirectories(src);
  }
  ~CookDirs() { FileSystem::removeAll(root); }
};

void
writePng(const FileSystemPath& path) {
  sf::Image image(sf::Vector2u{8u, 8u}, sf::Color::Blue);
  REQUIRE(image.saveToFile(path));
}

void
writeWav(const FileSystemPath& path) {
  constexpr unsigned kSampleCount = 64u;
  std::array<int16, kSampleCount> samples{};
  for (unsigned i = 0; i < kSampleCount; ++i) {
    samples[i] = static_cast<int16>(i * 100);
  }
  sf::SoundBuffer buffer;
  REQUIRE(buffer.loadFromSamples(samples.data(), kSampleCount, 1u, 44100u,
                                 {sf::SoundChannel::Mono}));
  REQUIRE(buffer.saveToFile(path));
}

void
writeBytes(const FileSystemPath& path, const char* text) {
  SPtr<DataStream> s = FileSystem::createAndOpenFile(path);
  REQUIRE(s != nullptr);
  s->write(text, std::char_traits<char>::length(text));
  s->close();
}

} // namespace

TEST_CASE("AssetCooker wraps a PNG into a .sfmxasset (no GL)") {
  CookDirs dirs("sfmx_cooker_png");
  writePng(dirs.src / "foo.png");

  const CookStats stats = AssetCooker::cookDirectory(dirs.src, dirs.out);
  CHECK(stats.cooked == 1);
  CHECK(stats.skipped == 0);

  const FileSystemPath cooked = dirs.out / "foo.sfmxasset";
  REQUIRE(FileSystem::exists(cooked));

  SPtr<DataStream> stream = FileSystem::openFile(cooked, AccessMode::kRead);
  AssetFileReader reader;
  REQUIRE(reader.open(stream));
  CHECK(reader.metadata().uuid.toString() ==
        sfmx::UUID::createFromName("foo.png").toString());
  CHECK(reader.metadata().assetType.toString() ==
        TypeTraits<TextureAsset>::getTypeId().toString());
  REQUIRE(reader.chunkCount() == 1);
  CHECK(reader.chunk(0).format == ChunkFormat::kPng);

  Vector<uint8> chunkBytes;
  REQUIRE(reader.readChunk(0, chunkBytes));
  const Vector<uint8> srcBytes = FileSystem::fastRead(dirs.src / "foo.png");
  CHECK(chunkBytes == srcBytes);
}

TEST_CASE("AssetCooker output loads through the AssetManager") {
  CookDirs dirs("sfmx_cooker_e2e");
  writePng(dirs.src / "hero.png");
  REQUIRE(AssetCooker::cookDirectory(dirs.src, dirs.out).cooked == 1);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<TextureCodec>());
  REQUIRE(mgr.mount(dirs.out) == 1u);

  SPtr<TextureAsset> asset =
      mgr.load<TextureAsset>(sfmx::UUID::createFromName("hero.png"));
  REQUIRE(asset != nullptr);
  CHECK(asset->isLoaded());
  CHECK(asset->texture().getSize() == sf::Vector2u{8u, 8u});
}

TEST_CASE("AssetCooker wraps a WAV into a SoundAsset") {
  CookDirs dirs("sfmx_cooker_wav");
  writeWav(dirs.src / "blip.wav");
  REQUIRE(AssetCooker::cookDirectory(dirs.src, dirs.out).cooked == 1);

  const FileSystemPath cooked = dirs.out / "blip.sfmxasset";
  REQUIRE(FileSystem::exists(cooked));

  SPtr<DataStream> stream = FileSystem::openFile(cooked, AccessMode::kRead);
  AssetFileReader reader;
  REQUIRE(reader.open(stream));
  CHECK(reader.metadata().assetType.toString() ==
        TypeTraits<SoundAsset>::getTypeId().toString());
  REQUIRE(reader.chunkCount() == 1);
  CHECK(reader.chunk(0).format == ChunkFormat::kWav);
  reader.close();

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<SoundCodec>());
  REQUIRE(mgr.mount(dirs.out) == 1u);
  SPtr<SoundAsset> asset =
      mgr.load<SoundAsset>(sfmx::UUID::createFromName("blip.wav"));
  REQUIRE(asset != nullptr);
  CHECK(asset->isLoaded());
}

TEST_CASE("AssetCooker skips unsupported files and cooks deterministically") {
  CookDirs dirs("sfmx_cooker_skip");
  writePng(dirs.src / "keep.png");
  writeBytes(dirs.src / "music.mp3", "not really mp3");
  writeBytes(dirs.src / "notes.txt", "hello");

  const CookStats stats = AssetCooker::cookDirectory(dirs.src, dirs.out);
  CHECK(stats.cooked == 1);                 // only keep.png
  CHECK(stats.skipped == 2);                // music.mp3 + notes.txt
  CHECK(FileSystem::exists(dirs.out / "keep.sfmxasset"));
  CHECK_FALSE(FileSystem::exists(dirs.out / "music.sfmxasset"));
  CHECK_FALSE(FileSystem::exists(dirs.out / "notes.sfmxasset"));

  // Cooking again into a second output yields byte-identical files (creationTime
  // is fixed at 0, the UUID is path-derived → reproducible cooks).
  const FileSystemPath out2 = dirs.root / "out2";
  REQUIRE(AssetCooker::cookFile(dirs.src / "keep.png", dirs.src, out2));
  const Vector<uint8> a = FileSystem::fastRead(dirs.out / "keep.sfmxasset");
  const Vector<uint8> b = FileSystem::fastRead(out2 / "keep.sfmxasset");
  REQUIRE_FALSE(a.empty());
  CHECK(a == b);
}
