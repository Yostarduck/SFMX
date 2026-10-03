#include <doctest/doctest.h>

#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"
#include "ui/UIFactory.h"
#include "ui/UIManager.h"
#include "ui/UIButton.h"
#include "ui/UICheckbox.h"
#include "ui/UITextBox.h"
#include "ui/UIVerticalBox.h"
#include "scripts/RegisterUIManager.h"
#include "utils/MemoryPoolHandler.h"

using namespace sfmx;

// Phase 6 — UIManager behaviour that Phase 2/3/4 could only assert by hand:
//  - selection lifecycle (select/deselect events, focus flags, same-widget
//    no-op, destroy-while-selected clears through forgetWidget);
//  - focus navigation WITHOUT the InputAction poll: moveSelection is public
//    for exactly this — selectFirst skips containers and reaches nested
//    children, directional scoring picks the best-aligned enabled candidate,
//    disabled widgets are never candidates, explicit nav links beat scoring
//    and fall back to it when their target is disabled;
//  - nested candidates are scored in ROOT space via toRootSpace — the nested
//    case is the regression net for the inverted transform (bug B);
//  - widget-level submit activation (button fires onPointerClick + onSubmit,
//    checkbox toggles through the identical click path);
//  - the full public text-input path: handleEvent consumed-ness, single
//    backspace deletion, Escape-while-focused fires onCancel and unfocuses;
//  - the Lua UI:createRoot factory (known kind roots + named lookup,
//    unknown kind returns nil).
//
// HEvent handles are reset (h = {}) BEFORE their emitter widget is destroyed —
// the Event system asserts if a connection outlives its emitter.

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

TEST_CASE("selection: events, focus flags, same-widget no-op, destroy clears") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();
  manager.setSelected(nullptr);

  UIButton* a = ui::createWidget<UIButton>(sf::Vector2f{120.f, 40.f});
  a->setPosition({0.f, 0.f});
  UIButton* b = ui::createWidget<UIButton>(sf::Vector2f{120.f, 40.f});
  b->setPosition({200.f, 0.f});
  manager.addRoot(a);
  manager.addRoot(b);

  int aSelect = 0, aDeselect = 0, bSelect = 0, bDeselect = 0;
  HEvent aSelH  = a->onSelect([&] { ++aSelect; });
  HEvent aDesH  = a->onDeselect([&] { ++aDeselect; });
  HEvent bSelH  = b->onSelect([&] { ++bSelect; });
  HEvent bDesH  = b->onDeselect([&] { ++bDeselect; });

  manager.setSelected(a);
  CHECK(manager.getSelected() == a);
  CHECK(aSelect == 1);
  CHECK(a->isFocused());
  CHECK_FALSE(b->isFocused());

  manager.setSelected(a);  // same widget: no-op, no duplicate events
  CHECK(manager.getSelected() == a);
  CHECK(aSelect == 1);

  manager.setSelected(b);
  CHECK(aDeselect == 1);
  CHECK_FALSE(a->isFocused());
  CHECK(bSelect == 1);
  CHECK(b->isFocused());

  manager.setSelected(nullptr);
  CHECK(bDeselect == 1);
  CHECK_FALSE(b->isFocused());
  CHECK(manager.getSelected() == nullptr);

  // Destroy-while-selected: forgetWidget drops the selection through the dtor.
  manager.setSelected(a);
  aSelH = {};  // reset every handle BEFORE any widget dies (Event contract)
  aDesH = {};
  bSelH = {};
  bDesH = {};
  destroyRoot(a);
  CHECK(manager.getSelected() == nullptr);

  destroyRoot(b);
  CHECK(manager.getSelected() == nullptr);
}

TEST_CASE("navigation: selectFirst + directional scoring + disabled skipped") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();
  manager.setSelected(nullptr);

  // First root is a container: selectFirst must skip it and reach its child.
  UIVerticalBox* box = ui::createWidget<UIVerticalBox>(sf::Vector2f{200.f, 150.f});
  box->setPosition({50.f, 50.f});
  UIButton* nested = ui::createWidget<UIButton>(sf::Vector2f{80.f, 40.f});
  nested->setPosition({10.f, 10.f});  // box-local
  box->addChild(nested);

  UIButton* a = ui::createWidget<UIButton>(sf::Vector2f{60.f, 30.f});
  a->setPosition({100.f, 100.f});  // centre (130,115)
  UIButton* b = ui::createWidget<UIButton>(sf::Vector2f{60.f, 30.f});
  b->setPosition({400.f, 100.f});  // directly right of a
  UIButton* c = ui::createWidget<UIButton>(sf::Vector2f{60.f, 30.f});
  c->setPosition({100.f, 400.f});  // directly below a
  UIButton* d = ui::createWidget<UIButton>(sf::Vector2f{60.f, 30.f});
  d->setPosition({250.f, 100.f});  // between a and b, but disabled
  d->setEnabled(false);

  manager.addRoot(box);
  manager.addRoot(a);
  manager.addRoot(b);
  manager.addRoot(c);
  manager.addRoot(d);

  // No selection -> selectFirst: containers skipped, nested child reached.
  manager.moveSelection({1.f, 0.f});
  CHECK(manager.getSelected() == nested);

  // Right from a: the aligned b wins; the disabled d is no candidate at all.
  manager.setSelected(a);
  manager.moveSelection({1.f, 0.f});
  CHECK(manager.getSelected() == b);

  // Down from a: c (b is exactly orthogonal -> dot == 0 -> skipped).
  manager.setSelected(a);
  manager.moveSelection({0.f, 1.f});
  CHECK(manager.getSelected() == c);

  // Dead selection (disabled) -> restart from the first selectable.
  manager.setSelected(d);
  manager.moveSelection({1.f, 0.f});
  CHECK(manager.getSelected() == nested);

  manager.setSelected(nullptr);
  destroyRoot(box);
  destroyRoot(a);
  destroyRoot(b);
  destroyRoot(c);
  destroyRoot(d);
}

TEST_CASE("navigation: explicit nav links beat scoring, disabled link falls back") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();
  manager.setSelected(nullptr);

  UIButton* a = ui::createWidget<UIButton>(sf::Vector2f{60.f, 30.f});
  a->setPosition({100.f, 100.f});
  UIButton* b = ui::createWidget<UIButton>(sf::Vector2f{60.f, 30.f});
  b->setPosition({400.f, 100.f});  // aligned right — would win by scoring
  UIButton* c = ui::createWidget<UIButton>(sf::Vector2f{60.f, 30.f});
  c->setPosition({100.f, 400.f});  // BELOW — linked explicitly
  UIButton* d = ui::createWidget<UIButton>(sf::Vector2f{60.f, 30.f});
  d->setPosition({700.f, 100.f});
  d->setEnabled(false);
  manager.addRoot(a);
  manager.addRoot(b);
  manager.addRoot(c);
  manager.addRoot(d);

  // Explicit link wins over the directionally-better b.
  a->setNavRight(c);
  manager.setSelected(a);
  manager.moveSelection({1.f, 0.f});
  CHECK(manager.getSelected() == c);

  // Link target disabled -> fall back to scoring -> aligned b.
  a->setNavRight(d);
  manager.setSelected(a);
  manager.moveSelection({1.f, 0.f});
  CHECK(manager.getSelected() == b);

  a->setNavRight(nullptr);
  manager.setSelected(nullptr);
  destroyRoot(a);
  destroyRoot(b);
  destroyRoot(c);
  destroyRoot(d);
}

TEST_CASE("navigation: nested candidates are scored in root space (bug B)") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();
  manager.setSelected(nullptr);

  UIButton* above = ui::createWidget<UIButton>(sf::Vector2f{60.f, 30.f});
  above->setPosition({100.f, 100.f});  // centre (130,115)
  UIVerticalBox* box = ui::createWidget<UIVerticalBox>(sf::Vector2f{300.f, 200.f});
  box->setPosition({100.f, 300.f});  // BELOW `above`, as a root
  UIButton* nested = ui::createWidget<UIButton>(sf::Vector2f{80.f, 40.f});
  nested->setPosition({20.f, 20.f});  // box-local -> root centre (160,340)
  box->addChild(nested);

  manager.addRoot(above);
  manager.addRoot(box);

  // The box is a container (never a candidate); the nested child is the only
  // candidate below — but only if toRootSpace maps its box-local centre into
  // ROOT space (forward transform). The inverted transform parks it at a
  // mirrored negative coordinate where dot(direction) <= 0 discards it, so
  // the selection would not move at all.
  manager.setSelected(above);
  manager.moveSelection({0.f, 1.f});
  CHECK(manager.getSelected() == nested);

  manager.setSelected(nullptr);
  destroyRoot(above);
  destroyRoot(box);
}

TEST_CASE("submit: base triggerSubmit activates button and toggles checkbox") {
  ensureUIEnv();

  UIButton* button = ui::createWidget<UIButton>(sf::Vector2f{150.f, 50.f});
  int clicks = 0, submits = 0;
  HEvent clickH = button->onPointerClick([&](sf::Vector2f) { ++clicks; });
  HEvent submitH = button->onSubmit([&] { ++submits; });

  // UIButton::triggerSubmit is a private override — call through the PUBLIC
  // base (virtual dispatch reaches the override either way).
  static_cast<UIWidget*>(button)->triggerSubmit();
  CHECK(submits == 1);
  CHECK(clicks == 1);  // submit fires the SAME click event a pointer press does

  UICheckbox* checkbox = ui::createWidget<UICheckbox>(sf::Vector2f{24.f, 24.f});
  int changes = 0, cbClicks = 0;
  bool lastValue = true;
  HEvent valueH = checkbox->onValueChanged([&](bool value) {
    ++changes;
    lastValue = value;
  });
  HEvent cbClickH = checkbox->onPointerClick([&](sf::Vector2f) { ++cbClicks; });
  CHECK_FALSE(checkbox->isChecked());

  static_cast<UIWidget*>(checkbox)->triggerSubmit();
  CHECK(checkbox->isChecked());
  CHECK(changes == 1);
  CHECK(lastValue);
  CHECK(cbClicks == 1);

  static_cast<UIWidget*>(checkbox)->triggerSubmit();
  CHECK_FALSE(checkbox->isChecked());
  CHECK(changes == 2);
  CHECK_FALSE(lastValue);
  CHECK(cbClicks == 2);

  clickH = {};
  submitH = {};
  valueH = {};
  cbClickH = {};
  destroyRoot(button);
  destroyRoot(checkbox);
}

TEST_CASE("handleEvent: textbox consumed matrix; Escape cancels and unfocuses") {
  ensureUIEnv();
  UIManager& manager = UIManager::instance();
  manager.setSelected(nullptr);

  UITextBox* box = ui::createWidget<UITextBox>(sf::Vector2f{200.f, 40.f});
  box->setPosition({10.f, 10.f});
  manager.addRoot(box);

  int cancels = 0;
  HEvent cancelH = box->onCancel([&] { ++cancels; });

  manager.setSelected(box);
  CHECK(manager.getSelected() == box);  // CHECK: keep cleanup running on failure

  // -- TextEntered with a focused editor: consumed + inserted --------------
  CHECK(manager.handleEvent(sf::Event{sf::Event::TextEntered{U'a'}}));
  CHECK(box->getText() == "a");

  // Backspace arrives as TEXT code point 8 — a single deletion.
  CHECK(manager.handleEvent(sf::Event{sf::Event::TextEntered{8}}));
  CHECK(box->getText().empty());

  // The Backspace KEY is consumed but must NOT delete a second time.
  box->setText("ab");
  box->moveCursorEnd();
  CHECK(manager.handleEvent(sf::Event{sf::Event::TextEntered{8}}));
  CHECK(box->getText() == "a");
  CHECK(manager.handleEvent(
      sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Backspace}}));
  CHECK(box->getText() == "a");

  // -- Cursor keys with a focused editor: consumed, cursor moves -----------
  CHECK(manager.handleEvent(
      sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Left}}));
  CHECK(box->getCursorPosition() == 0);

  box->setText("abc");
  box->moveCursorEnd();  // cursor 3
  CHECK(manager.handleEvent(
      sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Left}}));
  CHECK(box->getCursorPosition() == 2);
  CHECK(manager.handleEvent(
      sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Delete}}));
  CHECK(box->getText() == "ab");  // forward-delete removed 'c'
  CHECK(manager.handleEvent(
      sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Home}}));
  CHECK(box->getCursorPosition() == 0);
  CHECK(manager.handleEvent(
      sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::End}}));
  CHECK(box->getCursorPosition() == 2);

  // -- Escape with a focused editor: consumed, cancel fires, selection gone
  CHECK(manager.handleEvent(
      sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Escape}}));
  CHECK(cancels == 1);
  CHECK(manager.getSelected() == nullptr);
  CHECK_FALSE(box->isFocused());

  // -- No editor: everything falls through to the game ----------------------
  CHECK_FALSE(manager.handleEvent(sf::Event{sf::Event::TextEntered{U'x'}}));
  CHECK_FALSE(manager.handleEvent(
      sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Left}}));
  CHECK_FALSE(manager.handleEvent(
      sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Escape}}));
  CHECK_FALSE(manager.handleEvent(sf::Event{sf::Event::Resized{{800, 600}}}));
  CHECK(box->getText() == "ab");  // unconsumed events changed nothing

  cancelH = {};
  destroyRoot(box);
}

TEST_CASE("Lua UI:createRoot: known kind roots + named lookup, unknown nil") {
  ensureUIEnv();

  sol::state lua;
  lua.open_libraries(sol::lib::base);
  script::registerUIManager(sol::state_view(lua));

  sol::object uiGlobal = lua["UI"];
  REQUIRE(uiGlobal.get_type() != sol::type::nil);  // nothing created yet

  bool scriptOk = true;
  try {
    lua.script("created = UI:createRoot('button', 'LuaBtn');"
               "bogus = UI:createRoot('nosuchkind');");
  } catch (const sol::error& err) {
    scriptOk = false;
    FAIL(err.what());
  }

  sol::object created = lua["created"];
  sol::object bogus = lua["bogus"];
  if (scriptOk) {
    CHECK(created.get_type() != sol::type::nil);
    CHECK(bogus.get_type() == sol::type::nil);  // unknown kind -> nil

    // The created root is reachable through the manager's named lookup.
    UIWidget* found = UIManager::instance().findByName("LuaBtn");
    CHECK(found != nullptr);
    if (nullptr != found) {
      CHECK(found->getType() == WidgetType::kButton);
      destroyRoot(found);
    }
  }
}
