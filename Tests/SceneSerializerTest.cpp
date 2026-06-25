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
#include "ui/UIButton.h"
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

  if (!pools.hasPool<UIButton>()) {
    pools.registerPool<UIButton>(64);
  }

  if (!ComponentRegistry::isStarted()) {
    ComponentRegistry::startUp();
  }
  ComponentRegistry::instance().registerComponent<SceneDummyComponent>();
  ComponentRegistry::instance().registerComponent<UIButton>();
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

TEST_CASE("UIButton survives a full SceneSerializer round-trip") {
  ensureEnv();

  Scene src("src");
  SceneNode* n = src.createNode("ButtonHolder");
  REQUIRE(n != nullptr);

  UIButton* btn = n->addComponent<UIButton>(sf::Vector2f{300.f, 100.f});
  REQUIRE(btn != nullptr);

  // Customise widget state
  btn->setName("TestBtn");
  btn->setEnabled(true);
  btn->setVisible(true);
  btn->setInteractable(true);
  btn->setFocused(false);
  btn->setBlocksInput(true);
  btn->setPosition({50.f, 60.f});
  btn->setSize({300.f, 100.f});
  btn->setAnchorMin({0.1f, 0.2f});
  btn->setAnchorMax({0.9f, 0.8f});
  btn->setPivot({0.5f, 0.5f});
  btn->setColor(sf::Color(10, 20, 30, 200));

  // Custom collider
  btn->setColliderCircle({0.f, 0.f}, 40.f);

  // Customise button colour overrides
  btn->setNormalColor(sf::Color(255, 0, 0));
  btn->setHoveredColor(sf::Color(0, 255, 0));
  btn->setPressedColor(sf::Color(0, 0, 255));
  btn->setFocusedColor(sf::Color(255, 255, 0));
  btn->setDisabledColor(sf::Color(128, 128, 128));

  MemoryDataStream blob;
  REQUIRE(SceneSerializer::serialize(src, blob));
  blob.seek(0);

  Scene dst("dst");
  REQUIRE(SceneSerializer::deserialize(dst, blob));

  // Node recreated
  Vector<SceneNode*> found = dst.findNodesByName("ButtonHolder");
  REQUIRE(found.size() == 1u);

  // Button recreated
  UIButton* btn2 = found[0]->getComponent<UIButton>();
  REQUIRE(btn2 != nullptr);

  // Widget state
  CHECK(btn2->getName() == "TestBtn");
  CHECK(btn2->isEnabled());
  CHECK(btn2->isVisible());
  CHECK(btn2->isInteractable());
  CHECK_FALSE(btn2->isFocused());
  CHECK(btn2->isBlockingInput());

  CHECK(btn2->getPosition().x == doctest::Approx(50.f));
  CHECK(btn2->getPosition().y == doctest::Approx(60.f));
  CHECK(btn2->getSize().x == doctest::Approx(300.f));
  CHECK(btn2->getSize().y == doctest::Approx(100.f));

  CHECK(btn2->getAnchorMin().x == doctest::Approx(0.1f));
  CHECK(btn2->getAnchorMin().y == doctest::Approx(0.2f));
  CHECK(btn2->getAnchorMax().x == doctest::Approx(0.9f));
  CHECK(btn2->getAnchorMax().y == doctest::Approx(0.8f));
  CHECK(btn2->getPivot().x == doctest::Approx(0.5f));
  CHECK(btn2->getPivot().y == doctest::Approx(0.5f));

  CHECK(btn2->getColor().r == 10);
  CHECK(btn2->getColor().g == 20);
  CHECK(btn2->getColor().b == 30);
  CHECK(btn2->getColor().a == 200);

  // Collider
  const ICollider* col = btn2->getCollider();
  REQUIRE(col != nullptr);
  CHECK(col->getType() == ColliderType::kCircle);
  const auto* c = static_cast<const CircleCollider*>(col);
  CHECK(c->getCenter().x == doctest::Approx(0.f));
  CHECK(c->getCenter().y == doctest::Approx(0.f));
  CHECK(c->getRadius() == doctest::Approx(40.f));

  // Button colour overrides
  CHECK(btn2->getNormalColor() == sf::Color(255, 0, 0));
  CHECK(btn2->getHoveredColor() == sf::Color(0, 255, 0));
  CHECK(btn2->getPressedColor() == sf::Color(0, 0, 255));
  CHECK(btn2->getFocusedColor() == sf::Color(255, 255, 0));
  CHECK(btn2->getDisabledColor() == sf::Color(128, 128, 128));
}

TEST_CASE("UIButton round-trips no-collider state") {
  ensureEnv();

  Scene src("src");
  SceneNode* n2 = src.createNode("NoColliderBtn");
  REQUIRE(n2 != nullptr);

  UIButton* btn = n2->addComponent<UIButton>(sf::Vector2f{100.f, 50.f});
  REQUIRE(btn != nullptr);
  btn->clearCollider();
  btn->setBlocksInput(false);

  MemoryDataStream blob;
  REQUIRE(SceneSerializer::serialize(src, blob));
  blob.seek(0);

  Scene dst("dst");
  REQUIRE(SceneSerializer::deserialize(dst, blob));

  Vector<SceneNode*> found = dst.findNodesByName("NoColliderBtn");
  REQUIRE(found.size() == 1u);

  UIButton* btn2 = found[0]->getComponent<UIButton>();
  REQUIRE(btn2 != nullptr);
  CHECK_FALSE(btn2->isBlockingInput());
  CHECK(btn2->getCollider() == nullptr);
}
