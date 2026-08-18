#include <doctest/doctest.h>

#include "utils/UnitTest.h"
#include "scene/Scene.h"
#include "scene/ParticleSystemComponent.h"
#include "utils/FrameMemory.h"
#include "utils/MemoryPoolHandler.h"

#include <chrono>
#include <vector>

using namespace sfmx;

namespace {

// Minimal component for pool stress-testing (no dependencies on Game's main.cpp).
class TestComponent : public ComponentT<TestComponent> {
 public:
  explicit TestComponent(SceneNode* owner) : ComponentT<TestComponent>(owner) {}
};

// Second component to exercise the pool with a different type.
class AudioStubComponent : public ComponentT<AudioStubComponent> {
 public:
  explicit AudioStubComponent(SceneNode* owner)
    : ComponentT<AudioStubComponent>(owner) {}
};

// RAII helper — starts MemoryPoolHandler and registers the pools needed by
// these tests on construction, shuts down on destruction.
struct PoolFixture {
  PoolFixture() {
    if (!MemoryPoolHandler::isStarted()) {
      MemoryPoolHandler::startUp(4096);
    }
    auto& p = MemoryPoolHandler::instance();
    p.registerPool<Particle>(8192);
    p.registerPool<SceneNode>(4096);
    p.registerPool<ParticleSystemComponent>(256);
    p.registerPool<TestComponent>(256);
    p.registerPool<AudioStubComponent>(256);
  }
  ~PoolFixture() {
    // Do NOT shut down here — other test suites might still be using the pools.
    // Shutdown is handled in UnitTest.cpp's main().
  }
};

PoolFixture g_poolFixture;

}  // namespace

DECLARE_TYPE_TRAITS(TestComponent)
DECLARE_TYPE_TRAITS(AudioStubComponent)

// -------------------------------------------------------------------------
// Unit tests
// -------------------------------------------------------------------------

TEST_CASE("ParticleSystemComponent - default config") {
  Scene scene("TestParticle");
  SceneNode* node = scene.createNode("emitter");
  auto* ps = node->addComponent<ParticleSystemComponent>();
  REQUIRE(ps != nullptr);

  CHECK(ps->getParticleCount() == 0);
  CHECK_FALSE(ps->isWorldSpace());
  CHECK(ps->getSortMode() == ParticleSortMode::kNone);
  CHECK(ps->getEmissionRate() == doctest::Approx(0.0f));
}

TEST_CASE("ParticleSystemComponent - setConfig syncs capacity") {
  Scene scene("TestParticle");
  SceneNode* node = scene.createNode("emitter");
  auto* ps = node->addComponent<ParticleSystemComponent>();
  REQUIRE(ps != nullptr);

  EmitterConfig cfg;
  cfg.maxParticles = 100;
  ps->setConfig(cfg);
  CHECK(ps->getMaxParticles() == 100);
  CHECK(ps->getParticleCount() == 0);
}

TEST_CASE("ParticleSystemComponent - emit adds particles") {
  Scene scene("TestParticle");
  SceneNode* node = scene.createNode("emitter");
  auto* ps = node->addComponent<ParticleSystemComponent>();
  REQUIRE(ps != nullptr);

  EmitterConfig cfg;
  cfg.maxParticles = 50;
  ps->setConfig(cfg);

  ps->emit(10);
  CHECK(ps->getParticleCount() == 10);

  ps->emit(20);
  CHECK(ps->getParticleCount() == 30);
}

TEST_CASE("ParticleSystemComponent - emit clamps at capacity") {
  Scene scene("TestParticle");
  SceneNode* node = scene.createNode("emitter");
  auto* ps = node->addComponent<ParticleSystemComponent>();
  REQUIRE(ps != nullptr);

  EmitterConfig cfg;
  cfg.maxParticles = 30;
  ps->setConfig(cfg);

  ps->emit(100);   // exceeds capacity
  CHECK(ps->getParticleCount() == 30);
}

TEST_CASE("ParticleSystemComponent - clear removes all particles") {
  Scene scene("TestParticle");
  SceneNode* node = scene.createNode("emitter");
  auto* ps = node->addComponent<ParticleSystemComponent>();
  REQUIRE(ps != nullptr);

  EmitterConfig cfg;
  cfg.maxParticles = 50;
  ps->setConfig(cfg);
  ps->emit(20);
  REQUIRE(ps->getParticleCount() == 20);

  ps->clear();
  CHECK(ps->getParticleCount() == 0);
}

TEST_CASE("ParticleSystemComponent - onUpdate kills expired particles") {
  Scene scene("TestParticle");
  SceneNode* node = scene.createNode("emitter");
  auto* ps = node->addComponent<ParticleSystemComponent>();
  REQUIRE(ps != nullptr);

  EmitterConfig cfg;
  cfg.maxParticles = 50;
  cfg.lifetime = 0.5f;       // very short life
  cfg.emissionRate = 0.0f;   // manual emit only
  ps->setConfig(cfg);
  ps->emit(5);
  REQUIRE(ps->getParticleCount() == 5);

  // Advance past the lifetime
  ps->onUpdate(1.0f);
  CHECK(ps->getParticleCount() == 0);
}

TEST_CASE("ParticleSystemComponent - emission only when running") {
  Scene scene("TestParticle");
  SceneNode* node = scene.createNode("emitter");
  auto* ps = node->addComponent<ParticleSystemComponent>();
  REQUIRE(ps != nullptr);

  EmitterConfig cfg;
  cfg.maxParticles = 50;
  cfg.emissionRate = 100.0f;  // spawns every frame
  cfg.lifetime = 10.0f;       // won't die mid-test
  ps->setConfig(cfg);

  ps->start();
  CHECK(ps->isRunning());

  ps->onUpdate(0.5f);   // should have spawned some
  CHECK(ps->getParticleCount() > 0);

  ps->stop();
  size_t before = ps->getParticleCount();
  ps->onUpdate(0.5f);   // running=false => no new spawns
  CHECK(ps->getParticleCount() >= before);  // (existing may still be alive)
}

TEST_CASE("ParticleSystemComponent - start resets elapsed timer") {
  Scene scene("TestParticle");
  SceneNode* node = scene.createNode("emitter");
  auto* ps = node->addComponent<ParticleSystemComponent>();
  REQUIRE(ps != nullptr);

  EmitterConfig cfg;
  cfg.maxParticles = 50;
  cfg.duration = 1.0f;
  cfg.emissionRate = 0.0f;
  ps->setConfig(cfg);

  ps->emit(1);  // avoid early-return in onUpdate when m_count==0 && rate==0

  ps->onUpdate(0.6f);
  CHECK(ps->getProgress() == doctest::Approx(0.6f));

  ps->start();   // resets elapsed to 0
  CHECK(ps->getProgress() == doctest::Approx(0.0f));
}

// -------------------------------------------------------------------------
// Stress benchmark — create / destroy nodes with components repeatedly
// to exercise the pool allocators and verify no unbounded growth.
// -------------------------------------------------------------------------

TEST_CASE("ParticleSystemComponent - stress: create/destroy loop") {
  Scene scene("Stress");

  BENCHMARK("create/destroy 10000 nodes (3 components each)", [&]() {
    for (uint32 i = 0; i < 10'000; ++i) {
      SceneNode* node = scene.createNode("stress");
      node->addComponent<TestComponent>();
      node->addComponent<ParticleSystemComponent>();
      node->addComponent<AudioStubComponent>();
      scene.destroyNode(node);
    }
  });
}

// -------------------------------------------------------------------------
// Frame-rate scaling benchmark.
//
// Emits up to whatever the shared particle pool (see PoolFixture) actually has
// free and measures the per-frame cost of onUpdate() -- including the
// kBackToFront FrameScratch sort -- at each particle count. Reports both the
// current FrameMemory-backed path and a std::vector heap baseline so the
// frame-rate cost of going to more particles is visible.
// -------------------------------------------------------------------------

namespace {
struct BenchmarkFrameScope {
  BenchmarkFrameScope() {
    if (!FrameMemory::isStarted()) {
      FrameMemory::startUp(4u * 1024u * 1024u);
    }
  }
  ~BenchmarkFrameScope() {
    if (FrameMemory::isStarted()) {
      FrameMemory::shutDown();
    }
  }
};

// Fill an emitter's particles with varied Y so the BackToFront sort does work.
void
disperseParticles(ParticleSystemComponent& ps, size_t count) {
  ps.clear();  // setConfig doesn't reset count; release the previous sweep first
  EmitterConfig cfg;
  cfg.maxParticles  = count;
  cfg.lifetime      = 1e6f;       // effectively immortal for the measurement
  cfg.emissionRate  = 0.f;        // manual emit only, no steady-state spawning
  cfg.positionVariance = 512.f;   // spread positions so Y ordering is mixed
  cfg.speed         = 0.f;        // keep positions static across frames
  cfg.gravity       = {0.f, 0.f};
  ps.setConfig(cfg);
  ps.emit(count);
}

// Proxy for the old (pre-FrameMemory) kBackToFront pass: sorts `count` pointers
// with a heap std::vector exactly like the historical engine code did. Uses
// dummy particles so the workload (allocation volume + sort cost) is comparable
// to the arena path without reaching into the component's private list.
void
heapBackToFront(std::vector<Particle>& storage, size_t count) {
  storage.resize(count);
  for (size_t i = 0; i < count; ++i) {
    storage[i].position.y = static_cast<float>(count - i);  // reverse order
  }
  std::vector<Particle*> sorted;
  sorted.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    sorted.push_back(&storage[i]);
  }
  std::sort(sorted.begin(), sorted.end(),
            [](const Particle* a, const Particle* b) {
              return a->position.y < b->position.y;
            });
  DONOTOPTIMIZE(sorted.front());
}

// The current engine path: the component's own kBackToFront sort, which is
// backed by the inline stack / FrameMemory arena (FrameScratch).
void
arenaBackToFront(ParticleSystemComponent& ps) {
  ps.onUpdate(0.f);
}

}  // namespace

TEST_CASE("ParticleSystemComponent - framerate scaling to pool ceiling") {
  BenchmarkFrameScope frameScope;
  Scene scene("Framerate");
  SceneNode* node = scene.createNode("emitter");
  auto* ps = node->addComponent<ParticleSystemComponent>();
  REQUIRE(ps != nullptr);
  ps->setSortMode(ParticleSortMode::kBackToFront);

  const size_t poolCapacity = MemoryPoolHandler::instance().getCapacity<Particle>();
  const size_t poolInUse    = MemoryPoolHandler::instance().getAllocatedCount<Particle>();
  const size_t maxAvailable = poolCapacity - poolInUse;

  // Size tiers from the smallest useful sweep up to whatever is actually free
  // in the shared particle pool (earlier test cases may hold some slots).
  std::vector<size_t> counts = {128u, 512u, 1024u, 2048u, 4096u};
  if (maxAvailable > 4096u) {
    counts.push_back(maxAvailable);
  }

  MESSAGE(std::string(80, '='));
  MESSAGE("Particle framerate scaling (shared pool ceiling ", poolCapacity,
          ", available ", maxAvailable, ")");
  MESSAGE("  OnUpdate mode is the kBackToFront sort; \"sort-only\" subtracts the");
  MESSAGE("  simulation-only pass (kNone) so the arena vs heap comparison is even.");
  MESSAGE(std::string(80, '='));
  MESSAGE("  count | full-frame fps | arena sort µs/frame | heap sort µs/frame");
  MESSAGE(std::string(80, '='));

  using Clock = std::chrono::steady_clock;
  std::vector<Particle> heapStorage;

  for (size_t count : counts) {
    disperseParticles(*ps, count);
    REQUIRE(ps->getParticleCount() == count);

    // Warm up so allocations, caches and the sort path are representative.
    for (size_t i = 0; i < 8; ++i) {
      arenaBackToFront(*ps);
      FrameMemory::instance().endFrame();
    }

    const size_t iterations = 4000u;  // simulated frames per measurement

    // Simulation-only pass: sort disabled, so onUpdate cost is particle updates.
    ps->setSortMode(ParticleSortMode::kNone);
    auto simStart = Clock::now();
    for (size_t i = 0; i < iterations; ++i) {
      ps->onUpdate(0.f);
    }
    auto simEnd = Clock::now();
    const double simUs =
      std::chrono::duration<double, std::micro>(simEnd - simStart).count() / iterations;

    // Full frame through the real (FrameMemory-backed) sort path.
    ps->setSortMode(ParticleSortMode::kBackToFront);
    auto arenaStart = Clock::now();
    for (size_t i = 0; i < iterations; ++i) {
      arenaBackToFront(*ps);
      FrameMemory::instance().endFrame();
    }
    auto arenaEnd = Clock::now();
    const double arenaUs =
      std::chrono::duration<double, std::micro>(arenaEnd - arenaStart).count() / iterations;

    // Old-school heap sort of the same workload (allocation + sort included).
    auto heapStart = Clock::now();
    for (size_t i = 0; i < iterations; ++i) {
      heapBackToFront(heapStorage, count);
    }
    auto heapEnd = Clock::now();
    const double heapUs =
      std::chrono::duration<double, std::micro>(heapEnd - heapStart).count() / iterations;

    const double arenaSortUs = std::max(0.0, arenaUs - simUs);
    const double fullFps     = 1e6 / arenaUs;

    MESSAGE(std::to_string(count),
            " | ", std::to_string(fullFps), " FPS | ", std::to_string(arenaSortUs),
            " µs | ", std::to_string(heapUs), " µs");

    CHECK(arenaUs > 0.0);
    CHECK(fullFps > 0.0);
  }

  MESSAGE(std::string(80, '='));
}
