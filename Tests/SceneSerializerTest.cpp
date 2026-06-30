#include <doctest/doctest.h>

#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"
#include "core/MemoryDataStream.h"
#include "core/FileSystem.h"
#include "scene/ComponentRegistry.h"
#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "scene/SceneSerializer.h"
#include "scene/Transform.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// Serializable dummy component (registered with the ComponentRegistry).
struct SceneDummyComponent : ComponentT<SceneDummyComponent> {
  explicit SceneDummyComponent(SceneNode* owner)
    : ComponentT<SceneDummyComponent>(owner) {}

  void
  onSerialize(DataStream& stream) const override { stream << m_value << m_flag; }

  void
  onDeserialize(DataStream& stream) override { stream >> m_value >> m_flag; }

  int32 m_value = 0;
  uint8 m_flag  = 0;
};

// Serializable but deliberately NOT registered — exercises the unknown-type skip.
struct UnregisteredComp : ComponentT<UnregisteredComp> {
  explicit UnregisteredComp(SceneNode* owner)
    : ComponentT<UnregisteredComp>(owner) {}

  void
  onSerialize(DataStream& stream) const override { stream << m_x; }

  void
  onDeserialize(DataStream& stream) override { stream >> m_x; }

  float m_x = 0.f;
};

DECLARE_TYPE_TRAITS(SceneDummyComponent)
DECLARE_TYPE_TRAITS(UnregisteredComp)

namespace {

// Idempotent, never-shutdown setup (suite convention). Pools for SceneNode and
// both components; ComponentRegistry knows SceneDummyComponent but NOT
// UnregisteredComp (so loads of the latter are skipped cleanly).
void
ensureEnv() {
  if (!MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::startUp(4096);
  }
  MemoryPoolHandler& pools = MemoryPoolHandler::instance();
  if (!pools.hasPool<SceneNode>()) {
    pools.registerPool<SceneNode>(256);
  }
  if (!pools.hasPool<SceneDummyComponent>()) {
    pools.registerPool<SceneDummyComponent>(64);
  }
  if (!pools.hasPool<UnregisteredComp>()) {
    pools.registerPool<UnregisteredComp>(64);
  }

  if (!ComponentRegistry::isStarted()) {
    ComponentRegistry::startUp();
  }
  ComponentRegistry::instance().registerComponent<SceneDummyComponent>();
}

} // namespace

TEST_CASE("SceneSerializer round-trips hierarchy, transform, and components") {
  ensureEnv();

  Scene src("src");
  SceneNode* a = src.createNode("A");
  REQUIRE(a != nullptr);
  a->transform().setPosition({3.f, 4.f});
  a->transform().setRotation(sf::degrees(30.f));
  a->transform().setScale({2.f, 0.5f});
  a->setVisible(false);

  SceneDummyComponent* comp = a->addComponent<SceneDummyComponent>();
  REQUIRE(comp != nullptr);
  comp->m_value = 77;
  comp->m_flag  = 1;

  SceneNode* b = src.createNode("B", a);  // child of A
  REQUIRE(b != nullptr);
  src.createNode("C");                     // top-level sibling of A

  MemoryDataStream blob;
  REQUIRE(SceneSerializer::serialize(src, blob));
  blob.seek(0);

  Scene dst("dst");
  REQUIRE(SceneSerializer::deserialize(dst, blob));

  CHECK(dst.getNodeCount() == src.getNodeCount());

  // Hierarchy
  Vector<SceneNode*> foundA = dst.findNodesByName("A");
  REQUIRE(foundA.size() == 1u);
  SceneNode* a2 = foundA[0];
  CHECK(a2->getParent() == dst.getRoot());

  SceneNode* b2 = a2->findChild("B");
  REQUIRE(b2 != nullptr);
  CHECK(b2->getParent() == a2);

  Vector<SceneNode*> foundC = dst.findNodesByName("C");
  REQUIRE(foundC.size() == 1u);
  CHECK(foundC[0]->getParent() == dst.getRoot());

  // Flags + transform
  CHECK_FALSE(a2->isVisible());
  CHECK(a2->transform().getPosition().x == doctest::Approx(3.f));
  CHECK(a2->transform().getPosition().y == doctest::Approx(4.f));
  CHECK(a2->transform().getRotation().asDegrees() == doctest::Approx(30.f));
  CHECK(a2->transform().getScale().x == doctest::Approx(2.f));
  CHECK(a2->transform().getScale().y == doctest::Approx(0.5f));

  // Component round-trip
  SceneDummyComponent* comp2 = a2->getComponent<SceneDummyComponent>();
  REQUIRE(comp2 != nullptr);
  CHECK(comp2->m_value == 77);
  CHECK(comp2->m_flag == 1);
}

TEST_CASE("SceneSerializer skips components whose type is not registered") {
  ensureEnv();

  Scene src("src");
  SceneNode* d = src.createNode("D");
  REQUIRE(d != nullptr);
  UnregisteredComp* uc = d->addComponent<UnregisteredComp>();
  REQUIRE(uc != nullptr);
  uc->m_x = 9.f;

  MemoryDataStream blob;
  REQUIRE(SceneSerializer::serialize(src, blob));
  blob.seek(0);

  Scene dst("dst");
  REQUIRE(SceneSerializer::deserialize(dst, blob));  // must not hang/crash

  Vector<SceneNode*> foundD = dst.findNodesByName("D");
  REQUIRE(foundD.size() == 1u);
  // Node rebuilt, but the unregistered component was skipped.
  CHECK_FALSE(foundD[0]->hasComponent<UnregisteredComp>());
}

TEST_CASE("SceneSerializer round-trips through a .sfmxasset on disk") {
  ensureEnv();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_scene_test";
  FileSystem::removeAll(dir);
  const FileSystemPath path = dir / "level.sfmxasset";

  {
    Scene src("level");
    SceneNode* e = src.createNode("Emitter");
    REQUIRE(e != nullptr);
    e->transform().setPosition({10.f, 20.f});
    SceneDummyComponent* comp = e->addComponent<SceneDummyComponent>();
    REQUIRE(comp != nullptr);
    comp->m_value = 123;

    REQUIRE(SceneSerializer::saveToFile(src, path));
  }

  Scene dst("dst");
  REQUIRE(SceneSerializer::loadFromFile(dst, path));

  Vector<SceneNode*> found = dst.findNodesByName("Emitter");
  REQUIRE(found.size() == 1u);
  SceneNode* e2 = found[0];
  CHECK(e2->transform().getPosition().x == doctest::Approx(10.f));
  CHECK(e2->transform().getPosition().y == doctest::Approx(20.f));

  SceneDummyComponent* comp2 = e2->getComponent<SceneDummyComponent>();
  REQUIRE(comp2 != nullptr);
  CHECK(comp2->m_value == 123);

  FileSystem::removeAll(dir);
}

