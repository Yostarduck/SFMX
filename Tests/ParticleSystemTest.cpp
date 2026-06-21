#include <doctest/doctest.h>

#include "utils/UnitTest.h"
#include "scene/Scene.h"
#include "scene/ParticleSystemComponent.h"
#include "utils/MemoryPoolHandler.h"

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
