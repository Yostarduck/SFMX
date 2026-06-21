#include <doctest/doctest.h>

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"
#include "core/MemoryDataStream.h"
#include "scene/Component.h"
#include "scene/ComponentRegistry.h"
#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// A serializable component with trivial fields, to round-trip the hooks.
struct RegDummyComponent : ComponentT<RegDummyComponent> {
  explicit RegDummyComponent(SceneNode* owner)
    : ComponentT<RegDummyComponent>(owner) {}

  void
  onSerialize(DataStream& stream) const override {
    stream << m_value << m_x;
  }

  void
  onDeserialize(DataStream& stream) override {
    stream >> m_value >> m_x;
  }

  int32 m_value = 0;
  float m_x     = 0.f;
};

// A component that does NOT override the hooks, to check the default is inert.
struct RegPlainComponent : ComponentT<RegPlainComponent> {
  explicit RegPlainComponent(SceneNode* owner)
    : ComponentT<RegPlainComponent>(owner) {}
};

DECLARE_TYPE_TRAITS(RegDummyComponent)
DECLARE_TYPE_TRAITS(RegPlainComponent)

namespace {

// Idempotent, lazy setup. Follows the suite convention (see ParticleSystemTest's
// g_poolFixture): start the shared MemoryPoolHandler once and NEVER shut it down
// here — UnitTest.cpp's main() owns teardown, and other suites share the pools.
// Every registration is guarded so this is safe to call from each test and
// regardless of static-init order across translation units.
void
ensureEnv() {
  if (!MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::startUp(4096);
  }
  MemoryPoolHandler& pools = MemoryPoolHandler::instance();
  if (!pools.hasPool<SceneNode>()) {
    pools.registerPool<SceneNode>(256);
  }
  if (!pools.hasPool<RegDummyComponent>()) {
    pools.registerPool<RegDummyComponent>(64);
  }
  if (!pools.hasPool<RegPlainComponent>()) {
    pools.registerPool<RegPlainComponent>(64);
  }

  if (!ComponentRegistry::isStarted()) {
    ComponentRegistry::startUp();
  }
  ComponentRegistry::instance().registerComponent<RegDummyComponent>();
}

} // namespace

TEST_CASE("ComponentRegistry creates by type-id and round-trips the hooks") {
  ensureEnv();
  Scene scene("test");

  SceneNode* node = scene.createNode("source");
  REQUIRE(node != nullptr);
  RegDummyComponent* original = node->addComponent<RegDummyComponent>();
  REQUIRE(original != nullptr);
  original->m_value = 42;
  original->m_x     = 1.5f;

  MemoryDataStream stream;
  original->onSerialize(stream);
  stream.seek(0);

  // Rebuild a fresh component purely from its type id, then fill it.
  SceneNode* rebuilt = scene.createNode("rebuilt");
  REQUIRE(rebuilt != nullptr);
  Component* created =
      ComponentRegistry::instance().create(original->getTypeId(), rebuilt);
  REQUIRE(created != nullptr);
  CHECK(created->getTypeId() == original->getTypeId());

  created->onDeserialize(stream);
  RegDummyComponent* typed = static_cast<RegDummyComponent*>(created);
  CHECK(typed->m_value == 42);
  CHECK(typed->m_x == doctest::Approx(1.5f));
}

TEST_CASE("ComponentRegistry reports registration and rejects unknown types") {
  ensureEnv();
  ComponentRegistry& registry = ComponentRegistry::instance();

  CHECK(registry.isRegistered(componentTypeId<RegDummyComponent>()));
  CHECK_FALSE(registry.isRegistered(componentTypeId<RegPlainComponent>()));

  Scene scene("test");
  SceneNode* node = scene.createNode("n");
  // Unregistered type → clean nullptr.
  CHECK(registry.create(componentTypeId<RegPlainComponent>(), node) == nullptr);
  // Null node → clean nullptr.
  CHECK(registry.create(componentTypeId<RegDummyComponent>(), nullptr) == nullptr);
}

TEST_CASE("Component default serialize hooks write nothing") {
  ensureEnv();
  Scene scene("test");

  SceneNode* node = scene.createNode("plain");
  RegPlainComponent* plain = node->addComponent<RegPlainComponent>();
  REQUIRE(plain != nullptr);

  MemoryDataStream stream;
  plain->onSerialize(stream);  // base no-op
  CHECK(stream.size() == 0u);
}
