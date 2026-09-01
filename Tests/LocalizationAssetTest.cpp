#include <doctest/doctest.h>

#include "core/platform/Prerequisites.h"
#include "core/MemoryDataStream.h"
#include "core/FileSystem.h"
#include "assets/AssetFile.h"
#include "assets/AssetCooker.h"
#include "assets/AssetImporterRegistry.h"
#include "assets/AssetManager.h"
#include "assets/LocalizationAsset.h"
#include "assets/LocalizationCodec.h"
#include "utils/TypeTraits.h"

using namespace sfmx;

// Tab-separated localization table with the same layout as
// Game/resources/achievements_localization.csv: the header's first cell ("id") is
// skipped and the remaining header cells are the ids; each data row then starts
// with the language code followed by one value per id, in header order.

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

// RAII: the cooker consults the AssetImporterRegistry; the .csv -> LocalizationAsset
// import rule is one of the built-ins.
struct ImporterScope {
  ImporterScope() {
    if (AssetImporterRegistry::isStarted()) {
      AssetImporterRegistry::shutDown();
    }
    AssetImporterRegistry::startUp();
    AssetImporterRegistry::instance().registerBuiltins();
  }
  ~ImporterScope() {
    if (AssetImporterRegistry::isStarted()) {
      AssetImporterRegistry::shutDown();
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
writeText(const FileSystemPath& path, const ansichar* text) {
  SPtr<DataStream> s = FileSystem::createAndOpenFile(path);
  REQUIRE(s != nullptr);
  s->write(text, std::char_traits<ansichar>::length(text));
  s->close();
}

// The exact table shipped as Game/resources/achievements_localization.csv.
constexpr const ansichar* kAchievementsCsv =
    "id\tfirst_wave_n\tfirst_wave_d\n"
    "en\tFirst Wave\tComplete your first wave\n"
    "es\tPrimera Oleada\tCompleta tu primera oleada\n"
    "fr\tPremier tour\tTermine ton premier tour\n"
    "cz\tPrvní vlna\tDokonci první vlnu\n";

} // namespace

TEST_CASE("LocalizationAsset decodes tab-separated CSV into per-language maps") {
  AssetFileWriter writer;
  AssetMetadata meta;
  meta.assetType = TypeTraits<LocalizationAsset>::getTypeId();
  writer.setMetadata(meta);
  writer.addChunk(kAchievementsCsv, std::char_traits<ansichar>::length(kAchievementsCsv),
                  ChunkFormat::kCsv);

  auto buffer = MakeShared<MemoryDataStream>();
  REQUIRE(writer.writeTo(*buffer));
  buffer->seek(0);

  AssetFileReader reader;
  REQUIRE(reader.open(buffer));

  LocalizationCodec codec;
  SPtr<IAsset> asset = codec.decode(reader);
  REQUIRE(asset != nullptr);
  CHECK(asset->isLoaded());
  CHECK(asset->typeId() == TypeTraits<LocalizationAsset>::getTypeId());

  SPtr<LocalizationAsset> loc = std::static_pointer_cast<LocalizationAsset>(asset);
  const auto& table = loc->localizations();

  // The two id columns from the header.
  REQUIRE(table.size() == 2);
  REQUIRE(table.count("first_wave_n") == 1);
  REQUIRE(table.count("first_wave_d") == 1);

  // Each id is keyed by language code.
  const auto& firstWaveN = table.at("first_wave_n");
  CHECK(firstWaveN.size() == 4);
  CHECK(firstWaveN.at("en") == "First Wave");
  CHECK(firstWaveN.at("es") == "Primera Oleada");
  CHECK(firstWaveN.at("fr") == "Premier tour");

  const auto& firstWaveD = table.at("first_wave_d");
  CHECK(firstWaveD.size() == 4);
  CHECK(firstWaveD.at("en") == "Complete your first wave");
  CHECK(firstWaveD.at("es") == "Completa tu primera oleada");
  CHECK(firstWaveD.at("fr") == "Termine ton premier tour");
}

TEST_CASE("LocalizationAsset leaves empty cells out of the maps") {
  // Untranslated cells are left blank between the tabs; the parser should not
  // create entries (or languages) for them.
  const ansichar* csv =
      "id\thello\tworld\n"
      "de\tHallo\t\n"
      "it\t\tMondo\n";

  AssetFileWriter writer;
  AssetMetadata meta;
  meta.assetType = TypeTraits<LocalizationAsset>::getTypeId();
  writer.setMetadata(meta);
  writer.addChunk(csv, std::char_traits<ansichar>::length(csv), ChunkFormat::kCsv);

  auto buffer = MakeShared<MemoryDataStream>();
  REQUIRE(writer.writeTo(*buffer));
  buffer->seek(0);

  AssetFileReader reader;
  REQUIRE(reader.open(buffer));

  LocalizationCodec codec;
  SPtr<IAsset> asset = codec.decode(reader);
  REQUIRE(asset != nullptr);
  CHECK(asset->isLoaded());

  SPtr<LocalizationAsset> loc = std::static_pointer_cast<LocalizationAsset>(asset);
  const auto& table = loc->localizations();

  REQUIRE(table.size() == 2);
  const auto& hello = table.at("hello");
  const auto& world = table.at("world");
  CHECK(hello.size() == 1);
  CHECK(hello.count("de") == 1);
  CHECK(hello.at("de") == "Hallo");
  CHECK(world.size() == 1);
  CHECK(world.count("it") == 1);
  CHECK(world.at("it") == "Mondo");
}

TEST_CASE("LocalizationAsset fails cleanly on a chunk-less container") {
  AssetFileWriter writer;
  writer.setMetadata(AssetMetadata{});  // no chunks

  auto buffer = MakeShared<MemoryDataStream>();
  REQUIRE(writer.writeTo(*buffer));
  buffer->seek(0);

  AssetFileReader reader;
  REQUIRE(reader.open(buffer));

  LocalizationCodec codec;
  SPtr<IAsset> asset = codec.decode(reader);
  REQUIRE(asset != nullptr);
  CHECK_FALSE(asset->isLoaded());  // 0 chunks → kFailed
}

TEST_CASE("AssetCooker cooks a .csv and it loads through the AssetManager") {
  ImporterScope importers;
  CookDirs dirs("sfmx_localization_cook");
  writeText(dirs.src / "achievements.csv", kAchievementsCsv);

  const CookStats stats = AssetCooker::cookDirectory(dirs.src, dirs.out);
  CHECK(stats.cooked == 1);
  CHECK(stats.skipped == 0);

  const FileSystemPath cooked = dirs.out / "achievements.sfmxasset";
  REQUIRE(FileSystem::exists(cooked));

  // The cooked asset is tagged as a LocalizationAsset with a CSV chunk.
  SPtr<DataStream> stream = FileSystem::openFile(cooked, AccessMode::kRead);
  AssetFileReader reader;
  REQUIRE(reader.open(stream));
  CHECK(reader.metadata().assetType.toString() ==
        TypeTraits<LocalizationAsset>::getTypeId().toString());
  REQUIRE(reader.chunkCount() == 1);
  CHECK(reader.chunk(0).format == ChunkFormat::kCsv);
  reader.close();

  // End-to-end: mount the cooked dir and load the localization back by id.
  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<LocalizationCodec>());
  REQUIRE(mgr.mount(dirs.out) == 1u);

  SPtr<LocalizationAsset> loc =
      mgr.load<LocalizationAsset>(sfmx::UUID::createFromName("achievements.csv"));
  REQUIRE(loc != nullptr);
  CHECK(loc->isLoaded());
  CHECK(loc->localizations().at("first_wave_n").at("en") == "First Wave");
  CHECK(loc->localizations().at("first_wave_d").at("es") == "Completa tu primera oleada");
}