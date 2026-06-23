#include <doctest/doctest.h>

#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"
#include "core/MemoryDataStream.h"
#include "core/physics/Collider.h"
#include "core/physics/PhysicsSystem.h"
#include "scene/CameraComponent.h"
#include "scene/ColliderComponent.h"
#include "scene/ComponentRegistry.h"
#include "scene/ListenerComponent.h"
#include "scene/RigidBodyComponent.h"
#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "scene/SceneSerializer.h"
#include "scene/Transform.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// Tier A component serialization: Camera / Listener / RigidBody / Collider all
// carry pure value state (POD + a collider shape tag), so they round-trip with
// no asset or cross-component infrastructure. These tests exercise both the
// per-component hooks directly and the whole-scene path via SceneSerializer.

namespace {

// Idempotent, never-shutdown setup (suite convention — other suites share the
// global MemoryPoolHandler, so we never tear it down here).
void
ensureEnv() {
  if (!MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::startUp(4096);
  }
  MemoryPoolHandler& pools = MemoryPoolHandler::instance();
  if (!pools.hasPool<SceneNode>()) {
    pools.registerPool<SceneNode>(256);
  }
  if (!pools.hasPool<CameraComponent>()) {
    pools.registerPool<CameraComponent>(64);
  }
  if (!pools.hasPool<ListenerComponent>()) {
    pools.registerPool<ListenerComponent>(64);
  }
  if (!pools.hasPool<RigidBodyComponent>()) {
    pools.registerPool<RigidBodyComponent>(64);
  }
  if (!pools.hasPool<ColliderComponent>()) {
    pools.registerPool<ColliderComponent>(64);
  }

  if (!ComponentRegistry::isStarted()) {
    ComponentRegistry::startUp();
  }
  ComponentRegistry& reg = ComponentRegistry::instance();
  reg.registerComponent<CameraComponent>();
  reg.registerComponent<ListenerComponent>();
  reg.registerComponent<RigidBodyComponent>();
  reg.registerComponent<ColliderComponent>();
}

} // namespace

TEST_CASE("CameraComponent round-trips view, follow flag and draw order") {
  ensureEnv();

  Scene scene("cam");
  SceneNode* src = scene.createNode("src");
  SceneNode* dst = scene.createNode("dst");
  REQUIRE(src != nullptr);
  REQUIRE(dst != nullptr);

  CameraComponent* a = src->addComponent<CameraComponent>();
  REQUIRE(a != nullptr);
  a->setCenter({100.f, 200.f});
  a->setSize({640.f, 480.f});
  a->setRotation(45.f);
  a->setViewport(sf::FloatRect({0.1f, 0.2f}, {0.7f, 0.8f}));
  a->setFollowNode(false);
  a->setDrawOrder(7u);

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  CameraComponent* b = dst->addComponent<CameraComponent>();
  REQUIRE(b != nullptr);
  b->onDeserialize(blob);

  CHECK(b->getCenter().x == doctest::Approx(100.f));
  CHECK(b->getCenter().y == doctest::Approx(200.f));
  CHECK(b->getSize().x == doctest::Approx(640.f));
  CHECK(b->getSize().y == doctest::Approx(480.f));
  CHECK(b->getRotationDegrees() == doctest::Approx(45.f));
  CHECK(b->getViewport().position.x == doctest::Approx(0.1f));
  CHECK(b->getViewport().position.y == doctest::Approx(0.2f));
  CHECK(b->getViewport().size.x == doctest::Approx(0.7f));
  CHECK(b->getViewport().size.y == doctest::Approx(0.8f));
  CHECK_FALSE(b->isFollowingNode());
  CHECK(b->getDrawOrder() == 7u);
}

TEST_CASE("ListenerComponent round-trips the auto-update flag") {
  ensureEnv();

  Scene scene("listener");
  SceneNode* src = scene.createNode("src");
  SceneNode* dst = scene.createNode("dst");
  REQUIRE(src != nullptr);
  REQUIRE(dst != nullptr);

  ListenerComponent* a = src->addComponent<ListenerComponent>();
  REQUIRE(a != nullptr);
  a->setAutoUpdate(false);  // default is true, so this is a meaningful change

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  ListenerComponent* b = dst->addComponent<ListenerComponent>();
  REQUIRE(b != nullptr);
  b->onDeserialize(blob);

  CHECK_FALSE(b->isAutoUpdate());
}

TEST_CASE("RigidBodyComponent round-trips velocity, mass and gravity scale") {
  ensureEnv();

  Scene scene("rigid");
  SceneNode* src = scene.createNode("src");
  SceneNode* dst = scene.createNode("dst");
  REQUIRE(src != nullptr);
  REQUIRE(dst != nullptr);

  RigidBodyComponent* a = src->addComponent<RigidBodyComponent>();
  REQUIRE(a != nullptr);
  a->setVelocity({5.f, -3.f});
  a->setMass(2.5f);
  a->setGravityScale(0.5f);

  MemoryDataStream blob;
  a->onSerialize(blob);
  blob.seek(0);

  RigidBodyComponent* b = dst->addComponent<RigidBodyComponent>();
  REQUIRE(b != nullptr);
  b->onDeserialize(blob);

  CHECK(b->getVelocity().x == doctest::Approx(5.f));
  CHECK(b->getVelocity().y == doctest::Approx(-3.f));
  CHECK(b->getMass() == doctest::Approx(2.5f));
  CHECK(b->getGravityScale() == doctest::Approx(0.5f));
}

TEST_CASE("ColliderComponent round-trips each shape plus layer/flags/color") {
  ensureEnv();

  Scene scene("collider");

  SUBCASE("circle + layer/mask/flags/color") {
    SceneNode* src = scene.createNode("src");
    SceneNode* dst = scene.createNode("dst");
    REQUIRE(src != nullptr);
    REQUIRE(dst != nullptr);

    ColliderComponent* a = src->addComponent<ColliderComponent>();
    REQUIRE(a != nullptr);
    a->setCircle({1.f, 2.f}, 9.f);
    a->setLayer(PhysicsLayer::kEnemy);
    const LayerMask mask = static_cast<LayerMask>(PhysicsLayer::kPlayer)
                         | static_cast<LayerMask>(PhysicsLayer::kWorld);
    a->setCollisionMask(mask);
    a->setTrigger(true);
    a->setEnabled(false);
    a->setDebugColor(sf::Color(10, 20, 30, 40));

    MemoryDataStream blob;
    a->onSerialize(blob);
    blob.seek(0);

    ColliderComponent* b = dst->addComponent<ColliderComponent>();
    REQUIRE(b != nullptr);
    b->onDeserialize(blob);

    ICollider* shape = b->getCollider();
    REQUIRE(shape != nullptr);
    REQUIRE(shape->getType() == ColliderType::kCircle);
    const auto* c = static_cast<const CircleCollider*>(shape);
    CHECK(c->getCenter().x == doctest::Approx(1.f));
    CHECK(c->getCenter().y == doctest::Approx(2.f));
    CHECK(c->getRadius() == doctest::Approx(9.f));

    CHECK(b->getLayer() == PhysicsLayer::kEnemy);
    CHECK(b->getCollisionMask() == mask);
    CHECK(b->isTrigger());
    CHECK_FALSE(b->isEnabled());
    CHECK(b->getDebugColor() == sf::Color(10, 20, 30, 40));
  }

  SUBCASE("aabb") {
    SceneNode* src = scene.createNode("src");
    SceneNode* dst = scene.createNode("dst");
    ColliderComponent* a = src->addComponent<ColliderComponent>();
    REQUIRE(a != nullptr);
    a->setAABB({3.f, 4.f}, {16.f, 8.f});

    MemoryDataStream blob;
    a->onSerialize(blob);
    blob.seek(0);

    ColliderComponent* b = dst->addComponent<ColliderComponent>();
    REQUIRE(b != nullptr);
    b->onDeserialize(blob);

    ICollider* shape = b->getCollider();
    REQUIRE(shape != nullptr);
    REQUIRE(shape->getType() == ColliderType::kAABB);
    const auto* box = static_cast<const AABBCollider*>(shape);
    CHECK(box->getCenter().x == doctest::Approx(3.f));
    CHECK(box->getCenter().y == doctest::Approx(4.f));
    CHECK(box->getHalfSize().x == doctest::Approx(16.f));
    CHECK(box->getHalfSize().y == doctest::Approx(8.f));
  }

  SUBCASE("line") {
    SceneNode* src = scene.createNode("src");
    SceneNode* dst = scene.createNode("dst");
    ColliderComponent* a = src->addComponent<ColliderComponent>();
    REQUIRE(a != nullptr);
    a->setLine({-5.f, -6.f}, {7.f, 8.f});

    MemoryDataStream blob;
    a->onSerialize(blob);
    blob.seek(0);

    ColliderComponent* b = dst->addComponent<ColliderComponent>();
    REQUIRE(b != nullptr);
    b->onDeserialize(blob);

    ICollider* shape = b->getCollider();
    REQUIRE(shape != nullptr);
    REQUIRE(shape->getType() == ColliderType::kLine);
    const auto* line = static_cast<const LineCollider*>(shape);
    CHECK(line->getStart().x == doctest::Approx(-5.f));
    CHECK(line->getStart().y == doctest::Approx(-6.f));
    CHECK(line->getEnd().x == doctest::Approx(7.f));
    CHECK(line->getEnd().y == doctest::Approx(8.f));
  }
}

TEST_CASE("Tier A components survive a full SceneSerializer round-trip") {
  ensureEnv();

  Scene src("src");
  SceneNode* node = src.createNode("entity");
  REQUIRE(node != nullptr);

  RigidBodyComponent* rb = node->addComponent<RigidBodyComponent>();
  REQUIRE(rb != nullptr);
  rb->setVelocity({11.f, 22.f});
  rb->setMass(3.f);

  ColliderComponent* col = node->addComponent<ColliderComponent>();
  REQUIRE(col != nullptr);
  col->setAABB({0.f, 0.f}, {12.f, 6.f});
  col->setTrigger(true);

  MemoryDataStream blob;
  REQUIRE(SceneSerializer::serialize(src, blob));
  blob.seek(0);

  Scene dst("dst");
  REQUIRE(SceneSerializer::deserialize(dst, blob));

  Vector<SceneNode*> found = dst.findNodesByName("entity");
  REQUIRE(found.size() == 1u);
  SceneNode* n2 = found[0];

  RigidBodyComponent* rb2 = n2->getComponent<RigidBodyComponent>();
  REQUIRE(rb2 != nullptr);
  CHECK(rb2->getVelocity().x == doctest::Approx(11.f));
  CHECK(rb2->getVelocity().y == doctest::Approx(22.f));
  CHECK(rb2->getMass() == doctest::Approx(3.f));

  ColliderComponent* col2 = n2->getComponent<ColliderComponent>();
  REQUIRE(col2 != nullptr);
  REQUIRE(col2->getCollider() != nullptr);
  CHECK(col2->getCollider()->getType() == ColliderType::kAABB);
  CHECK(col2->isTrigger());
}
