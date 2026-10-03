#include <doctest/doctest.h>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/platform/Prerequisites.h"
#include "core/DataStream.h"
#include "core/DataStreamTypes.h"  // `stream << UUID`
#include "core/FileSystem.h"
#include "core/MemoryDataStream.h"
#include "ui/UIButton.h"
#include "ui/UICheckbox.h"
#include "ui/UIFactory.h"
#include "ui/UIHorizontalBox.h"
#include "ui/UIImage.h"
#include "ui/UILabel.h"
#include "ui/UIScrollView.h"
#include "ui/UISerializer.h"
#include "ui/UISlider.h"
#include "ui/UITextBox.h"
#include "ui/UIVerticalBox.h"
#include "ui/UIWidgetRegistry.h"
#include "utils/MemoryPoolHandler.h"
#include "utils/TypeTraits.h"

using namespace sfmx;

// Phase 5 — UI documents. A widget tree round-trips through UISerializer:
// records are [type UUID][payload byte size][child count][payload] written
// depth-first; payloads are UIWidget::onSerialize output (widget version byte
// + UIWidget::serializeBase shared state + type-specific fields). Covers the
// full 9-type nested tree, uniform base state (name/rect/flags/anchors now
// survive for EVERY widget — UILabel/UIImage used to drop it), unknown-type
// skipping, the widget version byte, and the .sfmxasset container on disk.

namespace {

// Idempotent, never-shutdown setup (suite convention): pools + factories.
void
ensureUIEnv() {
  if (!MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::startUp(4096);
  }
  MemoryPoolHandler& pools = MemoryPoolHandler::instance();
  if (!pools.hasPool<UIButton>())        { pools.registerPool<UIButton>(64); }
  if (!pools.hasPool<UILabel>())         { pools.registerPool<UILabel>(64); }
  if (!pools.hasPool<UIImage>())         { pools.registerPool<UIImage>(64); }
  if (!pools.hasPool<UICheckbox>())      { pools.registerPool<UICheckbox>(64); }
  if (!pools.hasPool<UITextBox>())       { pools.registerPool<UITextBox>(64); }
  if (!pools.hasPool<UISlider>())        { pools.registerPool<UISlider>(64); }
  if (!pools.hasPool<UIVerticalBox>())   { pools.registerPool<UIVerticalBox>(16); }
  if (!pools.hasPool<UIHorizontalBox>()) { pools.registerPool<UIHorizontalBox>(16); }
  if (!pools.hasPool<UIScrollView>())    { pools.registerPool<UIScrollView>(16); }

  if (!UIWidgetRegistry::isStarted()) {
    UIWidgetRegistry::startUp();
  }
  UIWidgetRegistry& reg = UIWidgetRegistry::instance();
  reg.registerWidget<UIButton>();
  reg.registerWidget<UILabel>();
  reg.registerWidget<UIImage>();
  reg.registerWidget<UICheckbox>();
  reg.registerWidget<UITextBox>();
  reg.registerWidget<UISlider>();
  reg.registerWidget<UIVerticalBox>();
  reg.registerWidget<UIHorizontalBox>();
  reg.registerWidget<UIScrollView>();
}

// Destroy a root through its pool — the only sanctioned path (never `delete`).
void
destroyRoot(UIWidget* widget) {
  if (nullptr != widget && MemoryPoolHandler::isStarted()) {
    MemoryPoolHandler::instance().deallocate(widget->getTypeId(),
                                              static_cast<void*>(widget));
  }
}

void
destroyAll(Vector<UIWidget*>& widgets) {
  for (UIWidget* widget : widgets) {
    destroyRoot(widget);
  }
  widgets.clear();
}

void
checkRect(const UIWidget& w, const sf::FloatRect& r) {
  CHECK(w.getRect().position.x == doctest::Approx(r.position.x));
  CHECK(w.getRect().position.y == doctest::Approx(r.position.y));
  CHECK(w.getRect().size.x     == doctest::Approx(r.size.x));
  CHECK(w.getRect().size.y     == doctest::Approx(r.size.y));
}

void
checkSlot(const UIWidget& w, const UISlot& s) {
  CHECK(w.getSlot().anchorMin.x == doctest::Approx(s.anchorMin.x));
  CHECK(w.getSlot().anchorMin.y == doctest::Approx(s.anchorMin.y));
  CHECK(w.getSlot().anchorMax.x == doctest::Approx(s.anchorMax.x));
  CHECK(w.getSlot().anchorMax.y == doctest::Approx(s.anchorMax.y));
  CHECK(w.getSlot().pivot.x     == doctest::Approx(s.pivot.x));
  CHECK(w.getSlot().pivot.y     == doctest::Approx(s.pivot.y));
  CHECK(w.getSlot().offset.x    == doctest::Approx(s.offset.x));
  CHECK(w.getSlot().offset.y    == doctest::Approx(s.offset.y));
}

// The 9-widget-type nested sample tree (2 roots); caller destroys the roots.
Vector<UIWidget*>
buildFullTree() {
  Vector<UIWidget*> roots;

  // Root 1: anchored button carrying a nested box subtree.
  UIButton* menu = ui::createWidget<UIButton>(sf::Vector2f{200.f, 50.f});
  menu->setName("MenuBtn");
  menu->setPosition({1055.f, 645.f});   // authored position first …
  menu->setAnchorMin({1.f, 1.f});       // … anchors + anchor-relative offset
  menu->setAnchorMax({1.f, 1.f});       //     LAST (slot authoring rule)
  menu->setPivot({1.f, 1.f});
  menu->setOffset({-25.f, -25.f});
  menu->setEnabled(false);
  menu->setColor(sf::Color(10, 20, 30, 200));
  roots.push_back(menu);

  UIVerticalBox* vbox =
      ui::createWidget<UIVerticalBox>(sf::Vector2f{300.f, 400.f});
  vbox->setName("VBox");
  vbox->setPosition({30.f, 60.f});
  vbox->setSpacing(7.f);
  vbox->setPadding({3.f, 9.f});
  menu->addChild(vbox);

  UILabel* label = ui::createWidget<UILabel>(sf::Vector2f{120.f, 30.f});
  label->setName("Label");
  label->setText("Hello UI");
  vbox->addChild(label);

  UICheckbox* check = ui::createWidget<UICheckbox>(sf::Vector2f{24.f, 24.f});
  check->setName("Check");
  check->setChecked(true, false);
  vbox->addChild(check);

  UISlider* slider = ui::createWidget<UISlider>(sf::Vector2f{180.f, 20.f});
  slider->setName("Slider");
  slider->setRange(0.f, 10.f);
  slider->setValue(4.5f, false);
  vbox->addChild(slider);

  UITextBox* editor = ui::createWidget<UITextBox>(sf::Vector2f{160.f, 28.f});
  editor->setName("Editor");
  editor->setText("type here");
  vbox->addChild(editor);

  UIHorizontalBox* hbox =
      ui::createWidget<UIHorizontalBox>(sf::Vector2f{280.f, 60.f});
  hbox->setName("HBox");
  hbox->setSpacing(4.f);
  hbox->setPadding({8.f, 2.f});
  vbox->addChild(hbox);

  UIImage* image = ui::createWidget<UIImage>(sf::Vector2f{48.f, 48.f});
  image->setName("Image");
  // No AssetManager in tests — the id round-trips as pure data.
  image->setTextureAssetId(UUID::createFromName("uitest_tex"));
  image->setFlipX(true);
  hbox->addChild(image);

  // Root 2: scroll view with a nested box.
  UIScrollView* scroll = ui::createWidget<UIScrollView>(sf::Vector2f{260.f, 180.f});
  scroll->setName("Scroll");
  scroll->setPosition({400.f, 120.f});
  scroll->setScrollOffset(35.f);
  roots.push_back(scroll);

  UIVerticalBox* inner =
      ui::createWidget<UIVerticalBox>(sf::Vector2f{240.f, 600.f});
  inner->setName("InnerBox");
  scroll->addChild(inner);

  return roots;
}

// Assert every field of buildFullTree's twin (structure + base + type state).
void
checkFullTree(const Vector<UIWidget*>& roots) {
  REQUIRE(roots.size() == 2);

  // --- root 1: button — base state: name, rect, flags, slot, colour -------
  UIWidget* menu = roots[0];
  CHECK(menu->getType() == WidgetType::kButton);
  CHECK(menu->getName() == "MenuBtn");
  checkRect(*menu, {{1055.f, 645.f}, {200.f, 50.f}});
  UISlot menuSlot;
  menuSlot.anchorMin = {1.f, 1.f};
  menuSlot.anchorMax = {1.f, 1.f};
  menuSlot.pivot     = {1.f, 1.f};
  menuSlot.offset    = {-25.f, -25.f};
  checkSlot(*menu, menuSlot);
  CHECK_FALSE(menu->isEnabled());
  CHECK(menu->isVisible());
  CHECK(menu->isBlockingInput());
  CHECK(menu->getColor() == sf::Color(10, 20, 30, 200));

  const auto& menuChildren = menu->getChildren();
  REQUIRE(menuChildren.size() == 1);
  UIWidget* vbox = menuChildren[0];
  CHECK(vbox->getType() == WidgetType::kVerticalBox);
  CHECK(vbox->getName() == "VBox");
  checkRect(*vbox, {{30.f, 60.f}, {300.f, 400.f}});
  auto* vboxTyped = static_cast<UIVerticalBox*>(vbox);
  CHECK(vboxTyped->getSpacing() == doctest::Approx(7.f));
  CHECK(vboxTyped->getPadding().x == doctest::Approx(3.f));
  CHECK(vboxTyped->getPadding().y == doctest::Approx(9.f));

  const auto& vboxChildren = vbox->getChildren();
  REQUIRE(vboxChildren.size() == 5);

  // Label — rect + text (rect silently dropped before Phase 5).
  UIWidget* label = vboxChildren[0];
  CHECK(label->getType() == WidgetType::kLabel);
  CHECK(label->getName() == "Label");
  checkRect(*label, {{0.f, 0.f}, {120.f, 30.f}});
  CHECK(static_cast<UILabel*>(label)->getText() == "Hello UI");

  // Checkbox — base flags + checked state (own byte now).
  UIWidget* check = vboxChildren[1];
  CHECK(check->getType() == WidgetType::kCheckbox);
  CHECK(check->getName() == "Check");
  CHECK(static_cast<UICheckbox*>(check)->isChecked());

  // Slider — range + value + thumb size.
  UIWidget* slider = vboxChildren[2];
  CHECK(slider->getType() == WidgetType::kSlider);
  CHECK(slider->getName() == "Slider");
  CHECK(static_cast<UISlider*>(slider)->getValue() == doctest::Approx(4.5f));

  // TextBox — UTF content.
  UIWidget* editor = vboxChildren[3];
  CHECK(editor->getType() == WidgetType::kTextBox);
  CHECK(editor->getName() == "Editor");
  CHECK(static_cast<UITextBox*>(editor)->getText() == "type here");

  // HBox → Image — nested container + texture id + flip.
  UIWidget* hbox = vboxChildren[4];
  CHECK(hbox->getType() == WidgetType::kHorizontalBox);
  CHECK(hbox->getName() == "HBox");
  auto* hboxTyped = static_cast<UIHorizontalBox*>(hbox);
  CHECK(hboxTyped->getSpacing() == doctest::Approx(4.f));
  CHECK(hboxTyped->getPadding().x == doctest::Approx(8.f));
  CHECK(hboxTyped->getPadding().y == doctest::Approx(2.f));

  const auto& hboxChildren = hbox->getChildren();
  REQUIRE(hboxChildren.size() == 1);
  UIWidget* image = hboxChildren[0];
  CHECK(image->getType() == WidgetType::kImage);
  CHECK(image->getName() == "Image");
  auto* imageTyped = static_cast<UIImage*>(image);
  CHECK(imageTyped->getTextureAssetId() == UUID::createFromName("uitest_tex"));

  // --- root 2: scroll view — scroll offset + nested box -------------------
  UIWidget* scroll = roots[1];
  CHECK(scroll->getType() == WidgetType::kScrollView);
  CHECK(scroll->getName() == "Scroll");
  checkRect(*scroll, {{400.f, 120.f}, {260.f, 180.f}});
  CHECK(static_cast<UIScrollView*>(scroll)->getScrollOffset() ==
        doctest::Approx(35.f));

  const auto& scrollChildren = scroll->getChildren();
  REQUIRE(scrollChildren.size() == 1);
  UIWidget* inner = scrollChildren[0];
  CHECK(inner->getType() == WidgetType::kVerticalBox);
  CHECK(inner->getName() == "InnerBox");
}

} // namespace

TEST_CASE("UI document round-trips a full 9-type tree") {
  ensureUIEnv();

  Vector<UIWidget*> original = buildFullTree();
  REQUIRE(original.size() == 2);

  MemoryDataStream blob;
  REQUIRE(UISerializer::serialize(original, blob));

  MemoryDataStream in(blob.data(), blob.size());
  Vector<UIWidget*> loaded;
  REQUIRE(UISerializer::deserialize(loaded, in));

  checkFullTree(loaded);

  destroyAll(loaded);
  destroyAll(original);
}

TEST_CASE("UI document skips unknown type records without desyncing") {
  ensureUIEnv();

  UIButton* probe = ui::createWidget<UIButton>(sf::Vector2f{50.f, 40.f});
  probe->setName("KnownBtn");

  MemoryDataStream payload;
  probe->onSerialize(payload);

  // Unknown id: the abstract base's type id is never factory-registered.
  const UUID unknownId = TypeTraits<UIWidget>::getTypeId();

  MemoryDataStream blob;
  blob << static_cast<uint32>(2);  // two roots
  // Root 1 — unknown type: 4 junk payload bytes, no children.
  blob << unknownId;
  blob << static_cast<uint64>(4);
  blob << static_cast<uint32>(0);
  const uint8 junk[4] = {0xDE, 0xAD, 0xBE, 0xEF};
  blob.write(junk, sizeof(junk));
  // Root 2 — a real button record.
  blob << probe->getTypeId();
  blob << static_cast<uint64>(payload.size());
  blob << static_cast<uint32>(0);
  blob.write(payload.data(), payload.size());

  MemoryDataStream in(blob.data(), blob.size());
  Vector<UIWidget*> loaded;
  REQUIRE(UISerializer::deserialize(loaded, in));
  REQUIRE(loaded.size() == 1);                // unknown root dropped …
  CHECK(loaded[0]->getName() == "KnownBtn");  // … next record parsed cleanly
  CHECK(loaded[0]->getType() == WidgetType::kButton);

  destroyAll(loaded);
  destroyRoot(probe);
}

TEST_CASE("UI document: unknown widget version yields defaults, stream intact") {
  ensureUIEnv();

  UIButton* probe = ui::createWidget<UIButton>(sf::Vector2f{70.f, 30.f});
  probe->setName("VProbe");

  MemoryDataStream badPayload;
  badPayload << static_cast<uint32>(999);  // widget version we don't know
  const uint8 junk[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  badPayload.write(junk, sizeof(junk));

  MemoryDataStream goodPayload;
  probe->onSerialize(goodPayload);

  MemoryDataStream blob;
  blob << static_cast<uint32>(2);
  // Root 1 — known type, unknown version: created, but at defaults.
  blob << probe->getTypeId();
  blob << static_cast<uint64>(badPayload.size());
  blob << static_cast<uint32>(0);
  blob.write(badPayload.data(), badPayload.size());
  // Root 2 — valid record: must still parse (payload isolation).
  blob << probe->getTypeId();
  blob << static_cast<uint64>(goodPayload.size());
  blob << static_cast<uint32>(0);
  blob.write(goodPayload.data(), goodPayload.size());

  MemoryDataStream in(blob.data(), blob.size());
  Vector<UIWidget*> loaded;
  REQUIRE(UISerializer::deserialize(loaded, in));
  REQUIRE(loaded.size() == 2);
  CHECK(loaded[0]->getName().empty());  // version rejected → no base state
  CHECK(loaded[1]->getName() == "VProbe");

  destroyAll(loaded);
  destroyRoot(probe);
}

TEST_CASE("UI document round-trips through a .sfmxasset on disk") {
  ensureUIEnv();

  const FileSystemPath dir = FileSystem::tempDirectory() / "sfmx_ui_test";
  FileSystem::removeAll(dir);
  const FileSystemPath path = dir / "hud.sfmxasset";

  Vector<UIWidget*> original = buildFullTree();
  REQUIRE(original.size() == 2);
  REQUIRE(UISerializer::saveToFile(original, path));
  destroyAll(original);  // "delete it" — the in-memory UI is gone …

  Vector<UIWidget*> loaded;
  REQUIRE(UISerializer::loadFromFile(loaded, path));  // … and loads it back
  checkFullTree(loaded);

  destroyAll(loaded);
  FileSystem::removeAll(dir);
}
