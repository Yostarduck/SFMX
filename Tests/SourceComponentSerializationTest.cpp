#include <doctest/doctest.h>

#include <cstdio>

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/SoundChannel.hpp>

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"
#include "core/MemoryDataStream.h"
#include "core/FileSystem.h"
#include "assets/AssetFile.h"
#include "assets/AssetManager.h"
#include "assets/SoundAsset.h"
#include "assets/SoundCodec.h"
#include "scene/ComponentRegistry.h"
#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "scene/SceneSerializer.h"
#include "scene/SourceComponent.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// M5.3 — SourceComponent serialization + SoundAsset (the audio asset-handle).
// kSound serializes the SoundAsset UUID (re-resolved via AssetManager); kMusic
// serializes the file path (re-opened); playback params round-trip for both.
// sfmx::UUID is qualified because Windows <rpcdce.h> defines a global ::UUID.

namespace {

constexpr unsigned   kSampleRate  = 44100u;
constexpr std::uint64_t kSampleCnt = 256u;

void
ensureEnv() {
  if (!MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::startUp(4096);
  }
  MemoryPoolHandler& pools = MemoryPoolHandler::instance();
  if (!pools.hasPool<SceneNode>()) {
    pools.registerPool<SceneNode>(256);
  }
  if (!pools.hasPool<SourceComponent>()) {
    pools.registerPool<SourceComponent>(64);
  }

  if (!ComponentRegistry::isStarted()) {
    ComponentRegistry::startUp();
  }
  ComponentRegistry::instance().registerComponent<SourceComponent>();
}

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

// Generates a short mono WAV and returns its absolute path. Caller cleans the dir.
FileSystemPath
writeWav(const FileSystemPath& dir) {
  FileSystem::removeAll(dir);
  FileSystem::createDirectories(dir);  // sf::SoundBuffer::saveToFile won't make it
  Vector<int16> samples(static_cast<size_t>(kSampleCnt), 0);
  for (size_t i = 0; i < samples.size(); ++i) {
    samples[i] = static_cast<int16>((i % 64) * 200);  // tiny non-silent ramp
  }
  sf::SoundBuffer buffer;
  REQUIRE(buffer.loadFromSamples(samples.data(), kSampleCnt, 1u, kSampleRate,
                                 {sf::SoundChannel::Mono}));
  const FileSystemPath wav = dir / "s.wav";
  REQUIRE(buffer.saveToFile(wav));
  return wav;
}

// Wraps a WAV's bytes in a `.sfmxasset` tagged as a SoundAsset; returns the file.
void
writeSoundAsset(const FileSystemPath& dir, const sfmx::UUID& id) {
  const FileSystemPath wav = writeWav(dir);

  SPtr<DataStream> in = FileSystem::openFile(wav, AccessMode::kRead);
  REQUIRE(in != nullptr);
  Vector<uint8> bytes(in->size());
  in->read(bytes.data(), bytes.size());
  in->close();

  AssetFileWriter writer;
  AssetMetadata meta;
  meta.uuid      = id;
  meta.assetType = TypeTraits<SoundAsset>::getTypeId();
  std::snprintf(meta.name, sizeof(meta.name), "%s", "blip");
  writer.setMetadata(meta);
  writer.addChunk(bytes.data(), bytes.size(), ChunkFormat::kRaw);  // decode auto-detects

  SPtr<DataStream> out = FileSystem::createAndOpenFile(dir / "blip.sfmxasset");
  REQUIRE(out != nullptr);
  REQUIRE(writer.writeTo(*out));
  out->close();
}

} // namespace

TEST_CASE("SoundAsset decodes audio bytes through the codec") {
  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_soundasset_test";
  const sfmx::UUID id = sfmx::UUID::createRandom();
  writeSoundAsset(dir, id);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<SoundCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  SPtr<SoundAsset> sound = mgr.load<SoundAsset>(id);
  REQUIRE(sound != nullptr);
  CHECK(sound->isLoaded());
  CHECK(sound->buffer().getSampleCount() == kSampleCnt);
  CHECK(sound->buffer().getSampleRate() == kSampleRate);

  FileSystem::removeAll(dir);
}

TEST_CASE("SourceComponent round-trips a SoundAsset UUID and playback params") {
  ensureEnv();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_source_sound_test";
  const sfmx::UUID id = sfmx::UUID::createRandom();
  writeSoundAsset(dir, id);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<SoundCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  {
    Scene scene("s");
    SceneNode* src = scene.createNode("src");
    REQUIRE(src != nullptr);

    SourceComponent* a = src->addComponent<SourceComponent>();
    REQUIRE(a != nullptr);
    a->setSoundAsset(mgr.load<SoundAsset>(id));
    REQUIRE(a->getSoundAsset() != nullptr);
    a->setVolume(50.f);
    a->setPitch(1.5f);
    a->setLooping(true);
    a->setMinDistance(5.f);
    a->setAttenuation(2.f);

    MemoryDataStream blob;
    a->onSerialize(blob);
    blob.seek(0);

    SceneNode* dst = scene.createNode("dst");
    SourceComponent* b = dst->addComponent<SourceComponent>();
    REQUIRE(b != nullptr);
    b->onDeserialize(blob);

    CHECK(b->getSoundAssetId().toString() == id.toString());
    REQUIRE(b->getSoundAsset() != nullptr);
    CHECK(b->getVolume() == doctest::Approx(50.f));
    CHECK(b->getPitch() == doctest::Approx(1.5f));
    CHECK(b->isLooping());
    CHECK(b->getMinDistance() == doctest::Approx(5.f));
    CHECK(b->getAttenuation() == doctest::Approx(2.f));
  }

  FileSystem::removeAll(dir);
}

TEST_CASE("SourceComponent round-trips a streaming music path") {
  ensureEnv();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_source_music_test";
  const FileSystemPath wav = writeWav(dir);
  const String path = wav.string();

  {
    Scene scene("s");
    SceneNode* src = scene.createNode("src");
    SourceComponent* a = src->addComponent<SourceComponent>();
    REQUIRE(a != nullptr);
    REQUIRE(a->loadMusicFromFile(path));
    a->setVolume(30.f);
    a->setLooping(true);

    MemoryDataStream blob;
    a->onSerialize(blob);
    blob.seek(0);

    SceneNode* dst = scene.createNode("dst");
    SourceComponent* b = dst->addComponent<SourceComponent>();
    REQUIRE(b != nullptr);
    b->onDeserialize(blob);

    // Music is path-backed, not asset-backed.
    CHECK(b->getSoundAssetId().toString() == sfmx::UUID::null().toString());
    CHECK(b->getSource() != nullptr);  // re-opened the streaming source
    CHECK(b->getVolume() == doctest::Approx(30.f));
    CHECK(b->isLooping());
  }  // scene destroyed here → sf::Music releases the file before removeAll

  FileSystem::removeAll(dir);
}

TEST_CASE("SourceComponent sound survives a full SceneSerializer round-trip") {
  ensureEnv();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_source_scene_test";
  const sfmx::UUID id = sfmx::UUID::createRandom();
  writeSoundAsset(dir, id);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<SoundCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  {
    Scene src("src");
    SceneNode* node = src.createNode("speaker");
    REQUIRE(node != nullptr);
    SourceComponent* source = node->addComponent<SourceComponent>();
    REQUIRE(source != nullptr);
    source->setSoundAsset(mgr.load<SoundAsset>(id));
    REQUIRE(source->getSoundAsset() != nullptr);

    MemoryDataStream blob;
    REQUIRE(SceneSerializer::serialize(src, blob));
    blob.seek(0);

    Scene dst("dst");
    REQUIRE(SceneSerializer::deserialize(dst, blob));

    Vector<SceneNode*> found = dst.findNodesByName("speaker");
    REQUIRE(found.size() == 1u);
    SourceComponent* source2 = found[0]->getComponent<SourceComponent>();
    REQUIRE(source2 != nullptr);
    CHECK(source2->getSoundAssetId().toString() == id.toString());
    CHECK(source2->getSoundAsset() != nullptr);
  }

  FileSystem::removeAll(dir);
}
