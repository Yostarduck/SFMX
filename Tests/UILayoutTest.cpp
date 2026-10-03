#include <doctest/doctest.h>

#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"
#include "ui/UIFactory.h"
#include "ui/UIManager.h"
#include "ui/UIButton.h"
#include "ui/UISlot.h"
#include "ui/UIVerticalBox.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// Phase 6 — anchor layout (Phase 3):
//  - UISlot::computePosition is pure math: position = ref(viewport) + offset
//    - pivot * size, with ref = lerp(anchorMin*viewport, anchorMax*viewport,
//    pivot). Differing anchors only move the reference — the size is NEVER
//    stretched. Corners, centre and the stretch case are hand-computed below.
//  - Roots reflow through UIManager::relayout (and the sf::Event::Resized hook,
//    which returns false — the game still sees the resize). applySlot saves the
//    authored slot offset, applies setPosition (which rewrites the offset as a
//    side effect), then RESTORES it — so repeated resizes are idempotent.
//  - Boxes keep auto-layout: relayout touches roots only, children stay where
//    padding/spacing put them, and child anchors ride along unapplied.

namespace {

// Idempotent, never-shutdown setup (suite convention): pools + registry come
// up through UIManager::onStartUp itself.
void
ensureUIEnv() {
  if (!MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::startUp(4096);
  }
  if (!UIManager::isStarted()) {
    UIManager::startUp();
  }
}

// Destroy a root through its pool — the only sanctioned path (never `delete`).
void
destroyRoot(UIWidget* widget) {
  if (nullptr != widget && MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::instance().deallocate(widget->getTypeId(),
                                              static_cast<void*>(widget));
  }
}

} // namespace

TEST_CASE("UISlot::computePosition: corners, centre, stretch anchors") {
  const sf::Vector2f viewport{1280.f, 720.f};
  const sf::Vector2f size{200.f, 50.f};
  UISlot slot;

  // Top-left pin (default anchors + pivot): offset IS the absolute position.
  slot.offset = {30.f, 40.f};
  sf::Vector2f p = slot.computePosition(viewport, size);
  CHECK(p.x == doctest::Approx(30.f));
  CHECK(p.y == doctest::Approx(40.f));

  // Bottom-right pin: ref (1280,720) + offset (-10,-10) - pivot*size.
  slot = UISlot{};
  slot.anchorMin = {1.f, 1.f};
  slot.anchorMax = {1.f, 1.f};
  slot.pivot = {1.f, 1.f};
  slot.offset = {-10.f, -10.f};
  p = slot.computePosition(viewport, size);
  CHECK(p.x == doctest::Approx(1070.f));  // 1280 - 10 - 200
  CHECK(p.y == doctest::Approx(660.f));   // 720 - 10 - 50

  // Centre anchor + centre pivot: (640,360) - (100,25).
  slot = UISlot{};
  slot.anchorMin = {0.5f, 0.5f};
  slot.anchorMax = {0.5f, 0.5f};
  slot.pivot = {0.5f, 0.5f};
  p = slot.computePosition(viewport, size);
  CHECK(p.x == doctest::Approx(540.f));
  CHECK(p.y == doctest::Approx(335.f));

  // Top-right pin.
  slot = UISlot{};
  slot.anchorMin = {1.f, 0.f};
  slot.anchorMax = {1.f, 0.f};
  slot.pivot = {1.f, 0.f};
  slot.offset = {-15.f, 25.f};
  p = slot.computePosition(viewport, sf::Vector2f{180.f, 40.f});
  CHECK(p.x == doctest::Approx(1085.f));  // 1280 - 15 - 180
  CHECK(p.y == doctest::Approx(25.f));    // 0 + 25 - 0

  // Stretch case: horizontal band anchors (min != max). The reference lerps
  // between (0,360) and (1280,360) at pivot.x -> (640,360); the SIZE of the
  // widget is not part of the anchors' business — computePosition never
  // touches it (asserted on the widget below, too).
  slot = UISlot{};
  slot.anchorMin = {0.f, 0.5f};
  slot.anchorMax = {1.f, 0.5f};
  slot.pivot = {0.5f, 0.5f};
  slot.offset = {0.f, 10.f};
  p = slot.computePosition(viewport, sf::Vector2f{400.f, 60.f});
  CHECK(p.x == doctest::Approx(440.f));  // 640 + 0 - 200
  CHECK(p.y == doctest::Approx(340.f));  // 360 + 10 - 30
}

TEST_CASE("root reflow: relayout + Resized event; offset restored, size kept") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();

  // Slot authoring order: position first, anchors + anchor-relative offset last.
  UIButton* anchored = ui::createWidget<UIButton>(sf::Vector2f{200.f, 50.f});
  anchored->setPosition({1070.f, 660.f});
  anchored->setAnchorMin({1.f, 1.f});
  anchored->setAnchorMax({1.f, 1.f});
  anchored->setPivot({1.f, 1.f});
  anchored->setOffset({-10.f, -10.f});

  // Default anchors: offset mirrors the absolute position — must never move.
  UIButton* plain = ui::createWidget<UIButton>(sf::Vector2f{120.f, 40.f});
  plain->setPosition({50.f, 60.f});

  manager.addRoot(anchored);
  manager.addRoot(plain);

  manager.relayout({1280.f, 720.f});
  CHECK(anchored->getPosition().x == doctest::Approx(1070.f));
  CHECK(anchored->getPosition().y == doctest::Approx(660.f));
  CHECK(anchored->getOffset().x == doctest::Approx(-10.f));  // restored, not
  CHECK(anchored->getOffset().y == doctest::Approx(-10.f));  // clobbered by
                                                             // setPosition
  CHECK(plain->getPosition().x == doctest::Approx(50.f));
  CHECK(plain->getPosition().y == doctest::Approx(60.f));

  // Second resize: same authored slot against a bigger viewport.
  manager.relayout({1920.f, 1080.f});
  CHECK(anchored->getPosition().x == doctest::Approx(1710.f));  // 1920-10-200
  CHECK(anchored->getPosition().y == doctest::Approx(1020.f));  // 1080-10-50
  CHECK(anchored->getOffset().x == doctest::Approx(-10.f));
  CHECK(anchored->getOffset().y == doctest::Approx(-10.f));

  // The Resized event hooks the same reflow and is NOT consumed.
  CHECK_FALSE(manager.handleEvent(sf::Event{sf::Event::Resized{{800, 600}}}));
  CHECK(anchored->getPosition().x == doctest::Approx(590.f));  // 800-10-200
  CHECK(anchored->getPosition().y == doctest::Approx(540.f));  // 600-10-50

  // Anchors move the reference, never the size.
  CHECK(anchored->getSize().x == doctest::Approx(200.f));
  CHECK(anchored->getSize().y == doctest::Approx(50.f));

  destroyRoot(anchored);
  destroyRoot(plain);
}

TEST_CASE("box auto-layout: children unaffected by root relayout, slots ignored") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();

  UIVerticalBox* box = ui::createWidget<UIVerticalBox>(sf::Vector2f{310.f, 250.f});
  box->setPosition({40.f, 30.f});  // deliberately NOT the origin
  box->setPadding({10.f, 15.f});
  box->setSpacing(8.f);

  UIButton* first = ui::createWidget<UIButton>(sf::Vector2f{100.f, 40.f});
  first->setPosition({999.f, 999.f});  // overwritten by updateLayout
  UIButton* second = ui::createWidget<UIButton>(sf::Vector2f{120.f, 60.f});
  box->addChild(first);
  box->addChild(second);
  box->updateLayout();

  CHECK(first->getPosition().x == doctest::Approx(10.f));   // padding.x
  CHECK(first->getPosition().y == doctest::Approx(15.f));   // padding.y
  CHECK(second->getPosition().x == doctest::Approx(10.f));
  CHECK(second->getPosition().y == doctest::Approx(63.f));  // 15 + 40 + 8

  // Child anchors ride along but are NEVER applied — slots apply to roots only.
  first->setAnchorMin({1.f, 1.f});
  first->setAnchorMax({1.f, 1.f});
  first->setPivot({1.f, 1.f});
  first->setOffset({-5.f, -5.f});

  manager.addRoot(box);
  manager.relayout({1920.f, 1080.f});

  CHECK(box->getPosition().x == doctest::Approx(40.f));  // default anchors:
  CHECK(box->getPosition().y == doctest::Approx(30.f));  // position unchanged
  CHECK(first->getPosition().x == doctest::Approx(10.f));   // box-local, intact
  CHECK(first->getPosition().y == doctest::Approx(15.f));
  CHECK(second->getPosition().x == doctest::Approx(10.f));
  CHECK(second->getPosition().y == doctest::Approx(63.f));
  CHECK(first->getAnchorMax().x == doctest::Approx(1.f));  // slot carried…

  box->updateLayout();  // …and layout re-run is idempotent over it.
  CHECK(first->getPosition().x == doctest::Approx(10.f));
  CHECK(first->getPosition().y == doctest::Approx(15.f));

  destroyRoot(box);  // children die with their parent
}
