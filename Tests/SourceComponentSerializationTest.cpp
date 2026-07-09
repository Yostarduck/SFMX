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
#include "assets/MusicAsset.h"
#include "assets/MusicCodec.h"
#include "assets/SoundAsset.h"
#include "assets/SoundCodec.h"
#include "scene/ComponentRegistry.h"
#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "scene/SceneSerializer.h"
#include "scene/SourceComponent.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// SourceComponent serialization + audio asset-handles.
// kSound serializes a SoundAsset UUID (resident PCM). kMusic serializes a MusicAsset
// UUID (v2: encoded bytes resident, streamed on play via openFromMemory) — the old
// path form is v1 legacy, still read but no longer written. Playback params round-trip.
// The MusicAsset must outlive its sf::Music (openFromMemory does not copy); the kept
// SPtr guarantees it. sfmx::UUID is qualified because Windows <rpcdce.h> defines ::UUID.

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

// Wraps a WAV's bytes in a `.sfmxasset` tagged as a MusicAsset. Returns the raw bytes
// so a test can assert the asset kept them intact (no decode). The format tag is
// irrelevant to MusicAsset (it holds bytes as-is); openFromMemory needs valid audio.
Vector<uint8>
writeMusicAsset(const FileSystemPath& dir, const sfmx::UUID& id) {
  const FileSystemPath wav = writeWav(dir);

  SPtr<DataStream> in = FileSystem::openFile(wav, AccessMode::kRead);
  REQUIRE(in != nullptr);
  Vector<uint8> bytes(in->size());
  in->read(bytes.data(), bytes.size());
  in->close();

  AssetFileWriter writer;
  AssetMetadata meta;
  meta.uuid      = id;
  meta.assetType = TypeTraits<MusicAsset>::getTypeId();
  std::snprintf(meta.name, sizeof(meta.name), "%s", "song");
  writer.setMetadata(meta);
  writer.addChunk(bytes.data(), bytes.size(), ChunkFormat::kWav);

  SPtr<DataStream> out = FileSystem::createAndOpenFile(dir / "song.sfmxasset");
  REQUIRE(out != nullptr);
  REQUIRE(writer.writeTo(*out));
  out->close();
  return bytes;
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

TEST_CASE("MusicAsset keeps the encoded bytes resident without decoding") {
  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_musicasset_test";
  const sfmx::UUID id = sfmx::UUID::createRandom();
  const Vector<uint8> original = writeMusicAsset(dir, id);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<MusicCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  SPtr<MusicAsset> music = mgr.load<MusicAsset>(id);
  REQUIRE(music != nullptr);
  CHECK(music->isLoaded());
  // The point of a streaming asset: the bytes are the SOURCE encoding, untouched
  // (not PCM). They match the cooked chunk exactly.
  CHECK(music->bytes().size() == original.size());
  CHECK(music->bytes() == original);

  FileSystem::removeAll(dir);
}

TEST_CASE("SourceComponent round-trips a MusicAsset UUID and streams from memory") {
  ensureEnv();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_source_music_test";
  const sfmx::UUID id = sfmx::UUID::createRandom();
  writeMusicAsset(dir, id);

  ManagerScope scope;
  AssetManager& mgr = AssetManager::instance();
  mgr.registerCodec(MakeShared<MusicCodec>());
  REQUIRE(mgr.mount(dir) == 1u);

  {
    Scene scene("s");
    SceneNode* src = scene.createNode("src");
    SourceComponent* a = src->addComponent<SourceComponent>();
    REQUIRE(a != nullptr);

    // Bind via a local SPtr, then drop it: the component's kept-alive m_musicAsset
    // is what must keep the bytes valid for the sf::Music opened over them. If the
    // keep-alive were missing this would be a use-after-free waiting to happen.
    {
      SPtr<MusicAsset> local = mgr.load<MusicAsset>(id);
      REQUIRE(local != nullptr);
      a->setMusicAsset(local);
    }
    mgr.unload(id);  // evict the cache entry too — only the component holds it now

    REQUIRE(a->getMusicAsset() != nullptr);   // keep-alive survives
    REQUIRE(a->getSource() != nullptr);        // sf::Music opened from the bytes
    a->setVolume(30.f);
    a->setLooping(true);

    MemoryDataStream blob;
    a->onSerialize(blob);
    blob.seek(0);

    SceneNode* dst = scene.createNode("dst");
    SourceComponent* b = dst->addComponent<SourceComponent>();
    REQUIRE(b != nullptr);
    b->onDeserialize(blob);

    // Music is asset-backed (v2): the UUID round-trips and re-resolves.
    CHECK(b->getMusicAssetId().toString() == id.toString());
    CHECK(b->getSoundAssetId().toString() == sfmx::UUID::null().toString());
    CHECK(b->getVolume() == doctest::Approx(30.f));
    CHECK(b->isLooping());
  }  // scene destroyed here → sf::Music released before removeAll

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
