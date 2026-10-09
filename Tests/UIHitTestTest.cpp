#include <doctest/doctest.h>

#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"
#include "ui/UIFactory.h"
#include "ui/UIManager.h"
#include "ui/UIButton.h"
#include "ui/UIScrollView.h"
#include "ui/UIVerticalBox.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// Phase 6 — hit-test contract.
//  1. containsPoint: plain rect test in the widget's rect frame — only
//     clearly-interior / clearly-exterior points are asserted (SFML rect
//     edge-inclusivity is version territory, not ours).
//  2. hitTestInHierarchy recursion: the parent maps the point from ITS frame
//     into the child's frame with getChildTransform().getInverse() — the exact
//     inverse of the transform drawHierarchy composes (draw: child vertex ->
//     parent frame via getChildTransform; hit-test walks the same edge
//     backwards). A non-zero container used to apply the transform FORWARD,
//     which made every nested child of an offset container unhittable —
//     the offset / two-level / scroll cases below are the regression net
//     (bug A), the origin case is the control that must stay green either way.
//  3. UIManager::hitTest: roots iterated newest-first (reverse draw order);
//     a blocking hit wins immediately, a non-blocking hit is only a fallback;
//     invisible roots pass through; disabled / non-interactable widgets still
//     CONSUME — they are returned as blockers so nothing beneath them fires.

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
// The dtor takes its children down with it and auto-removes the root.
void
destroyRoot(UIWidget* widget) {
  if (nullptr != widget && MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::instance().deallocate(widget->getTypeId(),
                                              static_cast<void*>(widget));
  }
}

} // namespace

TEST_CASE("containsPoint: interior points true, exterior false") {
  ensureUIEnv();

  UIButton* widget = ui::createWidget<UIButton>(sf::Vector2f{200.f, 80.f});
  widget->setPosition({100.f, 50.f});  // rect = (100,50,200,80)

  CHECK(widget->containsPoint({100.f, 50.f}));        // position corner
  CHECK(widget->containsPoint({150.f, 90.f}));        // plain interior
  CHECK(widget->containsPoint({299.f, 129.f}));       // size - 1 (still inside)
  CHECK_FALSE(widget->containsPoint({301.f, 50.f}));  // beyond right edge
  CHECK_FALSE(widget->containsPoint({100.f, 131.f})); // beyond bottom edge
  CHECK_FALSE(widget->containsPoint({99.f, 49.f}));   // before position

  destroyRoot(widget);  // never wired: dtor just returns it to the pool
}

TEST_CASE("hit-test recursion: container at the origin (control)") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();

  UIVerticalBox* box = ui::createWidget<UIVerticalBox>(sf::Vector2f{300.f, 250.f});
  box->setPosition({0.f, 0.f});
  UIButton* child = ui::createWidget<UIButton>(sf::Vector2f{80.f, 40.f});
  child->setPosition({10.f, 10.f});  // box-local
  box->addChild(child);
  manager.addRoot(box);

  // Identity container: correct and buggy recursion coincide — this pins the
  // frame convention independent of the transform direction.
  CHECK(manager.hitTest({15.f, 15.f}) == child);   // over the child
  CHECK(manager.hitTest({5.f, 200.f}) == box);     // in box, off child
  CHECK(manager.hitTest({-1.f, 5.f}) == nullptr);  // outside everything

  destroyRoot(box);
}

TEST_CASE("hit-test recursion: non-zero container reaches its child (bug A)") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();

  UIVerticalBox* box = ui::createWidget<UIVerticalBox>(sf::Vector2f{300.f, 250.f});
  box->setPosition({100.f, 50.f});
  UIButton* child = ui::createWidget<UIButton>(sf::Vector2f{80.f, 40.f});
  child->setPosition({10.f, 10.f});  // box-local
  box->addChild(child);
  manager.addRoot(box);

  // Child occupies root space (110,60)-(190,100): the root-space point must be
  // pulled back through the INVERSE child transform or the hit falls to the box.
  CHECK(manager.hitTest({130.f, 70.f}) == child);
  CHECK(manager.hitTest({105.f, 55.f}) == box);   // box margin, off the child
  CHECK(manager.hitTest({99.f, 49.f}) == nullptr);

  // A disabled child still CONSUMES: the blocker is the child itself, so the
  // parent box underneath receives nothing (root space (20,320) -> box-local
  // (20,20) lands inside the child rect (10,10,80,40)).
  UIVerticalBox* box2 = ui::createWidget<UIVerticalBox>(sf::Vector2f{300.f, 150.f});
  box2->setPosition({0.f, 300.f});
  UIButton* disabledChild = ui::createWidget<UIButton>(sf::Vector2f{80.f, 40.f});
  disabledChild->setPosition({10.f, 10.f});
  disabledChild->setEnabled(false);
  box2->addChild(disabledChild);
  manager.addRoot(box2);

  CHECK(manager.hitTest({20.f, 320.f}) == disabledChild);  // consumes
  CHECK(manager.hitTest({200.f, 320.f}) == box2);          // parent gets the rest

  destroyRoot(box);
  destroyRoot(box2);
}

TEST_CASE("hit-test recursion: two nested containers (bug A)") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();

  UIVerticalBox* outer = ui::createWidget<UIVerticalBox>(sf::Vector2f{300.f, 250.f});
  outer->setPosition({50.f, 40.f});
  UIVerticalBox* inner = ui::createWidget<UIVerticalBox>(sf::Vector2f{200.f, 150.f});
  inner->setPosition({20.f, 15.f});  // outer-local
  UIButton* grandchild = ui::createWidget<UIButton>(sf::Vector2f{60.f, 30.f});
  grandchild->setPosition({5.f, 5.f});  // inner-local
  inner->addChild(grandchild);
  outer->addChild(inner);
  manager.addRoot(outer);

  // root space = outer(50,40) + inner(20,15) + button(5,5): every level must
  // invert its own child transform, or the hit lands in the wrong frame.
  CHECK(manager.hitTest({80.f, 65.f}) == grandchild);
  CHECK(manager.hitTest({55.f, 45.f}) == outer);  // inside outer, off inner
  CHECK(manager.hitTest({49.f, 39.f}) == nullptr);

  destroyRoot(outer);
}

TEST_CASE("hit-test recursion: ScrollView honours the scroll offset (bug A)") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();

  UIScrollView* scroll = ui::createWidget<UIScrollView>(sf::Vector2f{310.f, 250.f});
  scroll->setPosition({25.f, 100.f});
  scroll->setScrollOffset(40.f);
  UIButton* child = ui::createWidget<UIButton>(sf::Vector2f{80.f, 40.f});
  child->setPosition({10.f, 60.f});  // content space
  scroll->addChild(child);
  manager.addRoot(scroll);

  // Child transform = translate(pos + (0,-offset)) = (25,60), so content
  // point (15,65) renders at root (40,125) — the inverse must recover it.
  CHECK(manager.hitTest({40.f, 125.f}) == child);
  CHECK(manager.hitTest({40.f, 250.f}) == scroll);  // viewport, below the child
  CHECK(manager.hitTest({24.f, 100.f}) == nullptr); // outside the viewport

  destroyRoot(scroll);
}

TEST_CASE("hit-test: blocking wins, non-blocking falls through") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();

  UIButton* bottom = ui::createWidget<UIButton>(sf::Vector2f{200.f, 100.f});
  bottom->setPosition({0.f, 0.f});  // blocking by default
  UIButton* top = ui::createWidget<UIButton>(sf::Vector2f{200.f, 100.f});
  top->setPosition({50.f, 0.f});
  top->setBlocksInput(false);
  manager.addRoot(bottom);
  manager.addRoot(top);  // drawn last = topmost

  CHECK(manager.hitTest({60.f, 20.f}) == bottom);  // overlap: blocking beats non-blocking
  CHECK(manager.hitTest({240.f, 20.f}) == top);    // only non-blocking hit -> fallback
  CHECK(manager.hitTest({10.f, 20.f}) == bottom);  // only the bottom root
  CHECK(manager.hitTest({300.f, 500.f}) == nullptr);

  // Both blocking: the newest root wins immediately.
  top->setBlocksInput(true);
  CHECK(manager.hitTest({60.f, 20.f}) == top);

  // Both non-blocking: the topmost fallback is kept.
  bottom->setBlocksInput(false);
  CHECK(manager.hitTest({60.f, 20.f}) == top);

  destroyRoot(bottom);
  destroyRoot(top);
}

TEST_CASE("hit-test: root draw order and root removal") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();

  UIButton* bottom = ui::createWidget<UIButton>(sf::Vector2f{200.f, 100.f});
  bottom->setPosition({0.f, 0.f});
  UIButton* top = ui::createWidget<UIButton>(sf::Vector2f{200.f, 100.f});
  top->setPosition({100.f, 0.f});
  manager.addRoot(bottom);
  manager.addRoot(top);

  CHECK(manager.hitTest({150.f, 20.f}) == top);  // overlap: last root added on top
  CHECK(manager.hitTest({50.f, 20.f}) == bottom);

  destroyRoot(top);  // dtor auto-removes the root
  CHECK(manager.hitTest({150.f, 20.f}) == bottom);  // next one down is exposed

  destroyRoot(bottom);
}

TEST_CASE("hit-test: disabled and non-interactable consume, invisible passes through") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();

  UIButton* bottom = ui::createWidget<UIButton>(sf::Vector2f{200.f, 100.f});
  bottom->setPosition({0.f, 0.f});  // enabled, visible, interactable, blocking
  UIButton* top = ui::createWidget<UIButton>(sf::Vector2f{200.f, 100.f});
  top->setPosition({50.f, 0.f});
  manager.addRoot(bottom);
  manager.addRoot(top);

  // Disabled: the widget itself is returned as the blocker (consumes).
  top->setEnabled(false);
  CHECK(manager.hitTest({60.f, 20.f}) == top);

  // Disabled + non-blocking: nothing blocks here, search continues downward.
  top->setBlocksInput(false);
  CHECK(manager.hitTest({60.f, 20.f}) == bottom);

  // Invisible: fully transparent — the enabled bottom root is reached.
  top->setEnabled(true);
  top->setBlocksInput(true);
  top->setVisible(false);
  CHECK(manager.hitTest({60.f, 20.f}) == bottom);

  // Non-interactable: consumes again.
  top->setVisible(true);
  top->setInteractable(false);
  CHECK(manager.hitTest({60.f, 20.f}) == top);

  destroyRoot(bottom);
  destroyRoot(top);
}
