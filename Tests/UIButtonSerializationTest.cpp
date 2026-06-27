#include <doctest/doctest.h>

#include <SFML/System/Vector2.hpp>

#include "core/DataStream.h"
#include "core/MemoryDataStream.h"
#include "core/physics/Collider.h"
#include "scene/ComponentRegistry.h"
#include "scene/Scene.h"
#include "scene/SceneNode.h"
#include "scene/SceneSerializer.h"
#include "ui/UIButton.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

namespace {

void
ensureEnv() {
  if (!MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::startUp(4096);
  }
  MemoryPoolHandler& pools = MemoryPoolHandler::instance();
  if (!pools.hasPool<SceneNode>()) {
    pools.registerPool<SceneNode>(256);
  }
  if (!pools.hasPool<UIButton>()) {
    pools.registerPool<UIButton>(64);
  }

  if (!ComponentRegistry::isStarted()) {
    ComponentRegistry::startUp();
  }
  ComponentRegistry::instance().registerComponent<UIButton>();
}

} // namespace

TEST_CASE("UIButton survives a full SceneSerializer round-trip") {
  ensureEnv();

  Scene src("src");
  SceneNode* n = src.createNode("ButtonHolder");
  REQUIRE(n != nullptr);

  UIButton* btn = n->addComponent<UIButton>(sf::Vector2f{300.f, 100.f});
  REQUIRE(btn != nullptr);

  // Customise widget state
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
