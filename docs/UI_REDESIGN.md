# UI Subsystem Redesign — Unreal-like Independence

**Branch:** `Fix/UIRevamped` (clean break — old UI paths are deleted, no adapter layer)
**Status:** Phase 1 **complete** — build green, ctest 155 cases / 154 pass (the 1 fail is
the known pre-existing `ParticleSystemTest`, unchanged), demo runs with migrated Lua,
layering gate empty. Phase 0 **dropped** as non-UI. Phases 2–6 open.

## Locked decisions

| # | Topic | Decision |
|---|-------|----------|
| 1 | Scene bridge | **None.** Widgets never touch `SceneNode`. `ui/` may depend on `core`, `gfx`, `input`, `assets` — never on `scene/`. |
| 2 | Allocation | **Keep pools.** Pool registration moves out of `Game/DemoScene.cpp` into the UI subsystem (`UIManager::onStartUp`). |
| 3 | Layout | **Simplest model:** per-child `UISlot` (anchors/pivot/offset), applied by an implicit viewport-spanning root panel. Boxes and ScrollView keep their current auto-layout. No UMG-style stretch/percent presets. |
| 4 | Serialization | **Separated UI documents** (own asset file, `AssetFileWriter`/`Reader` + LZ4, like scenes) — the widget-tree equivalent of a UMG widget blueprint. *(Assumption: "serialization would be great" = my recommended separate-document option. Say if you actually wanted a subtree section inside the scene file.)* |
| 5 | Migration | **Clean break** on this branch; each phase ends with a green build + green test suite. |
| 6 | Fluent builder syntax | **Undecided** — deferred; revisit as an optional nicety after Phase 4. |

### Assumptions to confirm (defaults used below)

- **Disabled/non-interactable widgets consume clicks** (standard UI: a dead button doesn't
  click what's under it). Falls through only when `blocksInput == false`.
- `blocksInput == false` means: **this widget receives the event AND lower widgets are also
  tested** (fixes the dead `fallback` branch in `Canvas::hitTest`).

## Target architecture

```
FrameworkGame/include/ui|source/ui        (NO scene/ includes allowed)
├── UIManager        — Module; replaces UIEventSystem + Canvas + CanvasComponent
│     · owns root widgets (depth-ordered, top root hit first)
│     · registers widget pools at startup; erased destroy via pool
│     · update(window, dt)  → pointer / scroll / navigation / selection
│     · handleEvent(event)  → returns true if UI consumed it (game input gated)
│     · draw(target)        → called in main loop AFTER postFx->render (crisp HUD)
│     · relayout(viewport)  → re-applies anchors on resize
├── UIWidget         — pure base: rect, state, events, name, hierarchy.
│                       No ComponentT, no SceneNode, no Collider (dead code removed).
│                       Children owned as pool-allocated raw ptrs, destroyed by parent.
├── UISlot           — anchorMin/anchorMax/pivot/offset per child
├── widgets          — Button, Label, Image, Checkbox(+Group), TextBox, Slider,
│                       VerticalBox, HorizontalBox, ScrollView (unchanged behavior)
├── UIWidgetRegistry — type UUID → factory (for document loading)
└── UISerializer     — save/load a widget tree to/from a standalone document
```

**Ownership rule:** every widget is either a root registered with `UIManager` or a child of
a panel. No orphans. Parent dtor → `MemoryPoolHandler::deallocate(typeId, child)` (same
erased-destroy pattern `SceneNode` uses for components).

## Phases

Each phase must end: `cmake --build Build --config Debug` clean **and** `ctest` all green
**and** the demo launches with the HUD working.

---

### Phase 0 — Baseline guardrail — **dropped (non-UI scope)**

> Scope decision: *"don't do work that is not related to the UI."* The pre-existing
> failure (4 failed CHECKs at `Tests/ParticleSystemTest.cpp:329-332`, "emit stamps
> custom data on each particle", broken in `2cb76be` when `emit(count, payload)` became
> `emit(count, optional<EmitterConfig>)`) is unrelated to this redesign and stays as-is.

- [x] Baseline recorded: 157 test cases, 156 pass / **1 pre-existing fail**
  (`ParticleSystemTest`) — that one failure remains the expected ctest state for the
  whole redesign (Phase 1 measures against it: 155 after the 2 deleted test cases).

**Result:** phase skipped; no non-UI work performed.

---

### Phase 1 — Core independence (the big-bang cutover) — **DONE**

Goal: `ui/` compiles with zero `scene/` includes; demo runs with behavior parity.

**Core:**
- [x] `UIWidget.h`: `ComponentT` dual-inheritance dropped (plain class); collider API,
      `m_collider` and `s_canvasDrawing` gone (`containsPoint` stays rect-only); added
      `m_name` + `setName/getName` (for Lua `UI:get(name)`), `onUpdate` /
      `onSerialize` / `onDeserialize` virtuals (UI-owned contract), `getManager()` chain
      walk; `Canvas`/`UIEventSystem`/`CanvasComponent` friends and back-pointer removed.
- [x] All 9 widget headers/sources stripped: no `scene/` includes, no `SceneNode*` ctor
      overloads, no `syncColliderToRect`. **Gate verified:**
      `grep -rn '#include "scene/' FrameworkGame/include/ui FrameworkGame/source/ui` → empty.
- [x] `CanvasComponent.h/.cpp`, `UIEventSystem.h/.cpp`, `ui/Canvas.h/.cpp` deleted;
      their jobs absorbed into `UIManager` (roots registry replacing the canvas list,
      selection + pointer/scroll/nav processing, hit-test blocking/fallback logic moved
      verbatim from `Canvas::hitTest` — dead-fallback fixed in Phase 2; `s_canvasDrawing`
      died with them — `UIManager::draw` draws directly).
- [x] `UIManager::onStartUp`: registers widget pools (`UIButton 64, UILabel 64, UIImage 64,
      UICheckbox 64, UITextBox 64, UISlider 64, UIVerticalBox 16, UIHorizontalBox 16,
      UIScrollView 16` — hasPool-guarded) + resets state.
- [x] Factory helper `ui::createWidget<T>(...)` (`ui/UIFactory.h`, header-only, pool
      allocate + `SFMX_ASSERT`); parents destroy children via erased
      `deallocate(UUID, void*)` with a dtor progress guard (pool deallocate silently
      no-ops on out-of-storage pointers).
- [x] `UIManager::draw`: called after `postFx->render(...)`; sets the default view
      itself and restores it afterwards (same trick `CanvasComponent::onDraw` used) so
      pixel coords match hit-test.

**Consumers updated in the same phase (compile-level, behavior parity):**
- [x] `Game/main.cpp`: no `scene.createNode("HUDCanvas")`, no `addComponent<UI…>`; widgets
      created via the factory **with names** ("DebugLabel", "UpgradesButton", "InfoLabel",
      "UpgradesMenu", "UpgradesList", `<Name> HBox/Label/Cost Label/Button`, "BuyLabel",
      "BuySlider", "ExitBtn", "ToggleShaderBtn", "ShaderLabel"), registered as roots via
      `UIManager::addRoot` / `parent->addChild`; `UIEventSystem::instance()` →
      `UIManager::instance()` (actions, TextEntered selection, update, draw, teardown);
      `UIManager::startUp()` **before** `ScriptEngine::startUp()` (the Lua `UI` global
      only binds when UIManager is up); teardown `UIManager::shutDown()` after scenes,
      before ScriptEngine and pools; HUD draw moved after `postFx->render` (commented-out
      `uiCanvas.draw` line retired into `UIManager::draw`).
  - *While touching this region:* `UILabel* debugLabel = nullptr` init **and** a null
    guard before `debugLabel->setText` — **done** (UI-owned lifetime bug).
  - *Deferred (non-UI, per scope decision):* the duplicate include (lines 60/63) and the
    FPS-average loop bug (`deltas[index]` should be used, not `deltas[i]`, lines 584-590).
- [x] `Game/DemoScene.cpp`: UI pools (137-146), UI `registerComponent` (181-189),
      pool-stat prints, UI includes — all removed (pools now owned by `UIManager`).
- [x] `FrameworkGame/source/scripts/RegisterAll.cpp`: `registerComponentType<UI…>()`
      (201-209) + widget includes removed; `RegisterUI*.cpp` ×10: `componentTypeId<…>()`
      stamps dropped, bases now `sol::bases<UIWidget>()`, `scene/Component.h` /
      `scene/SceneNode.h` includes removed (ScriptComponent kept where lambdas use it).
- [x] New `scripts/RegisterUIManager.h/.cpp`: `UI` usertype with `get` only (concrete-type
      switch on `getType()`), guarded `lua["UI"] = std::ref(...)`; wired into
      `RegisterAll.cpp` + `FrameworkGame/CMakeLists.txt`.
- [x] `Tests/UIButtonSerializationTest.cpp`: **deleted** — tested scene round-trip of a UI
      *component*, which no longer exists; replacement lands in Phase 5.
      Removed from `Tests/CMakeLists.txt`.
- [x] `SceneSerializer` needs no change: unknown component types already skipped
      (bytes consumed, attach nothing) — scene tests green.

**Verification (recorded at completion):**
- Build green: `cmake --build Build --config Debug` → exit 0.
- `ctest`: **155 cases, 154 pass / 1 fail** — the 1 fail is the known pre-existing
  `ParticleSystemTest` (157 baseline − 2 deleted test cases).
- Demo runs: cooked `gameManager.sfmxasset` re-cooked after the migration (asset mtimes
  verified newer than the edit), all 14 `UI:get(...)` lookups resolve — zero
  "…not found" prints, zero Lua errors.
- Layering gate empty (command above).

**Exit:** met — build green, ctest green modulo the known pre-existing failure, demo
launches, HUD renders and is clickable. Intentional visual change: the HUD now draws
*after* the post-FX chain (crisp pixels) instead of through it.

---

### Phase 2 — Interaction correctness (fix the known bugs in the new core) — **DONE**

- [x] **Submit activates:** `UIButton::triggerSubmit` flashes the pressed visual while the
      submit event fires, restores the normal/focused state, then fires the same
      `triggerPointerClick` event a pointer press+release would; `UICheckbox::triggerSubmit`
      reuses the full `triggerPointerClick` path (group exclusivity included). (Was: the
      old system fired an event nobody handled — keyboard/gamepad could focus but never
      click.)
- [x] **TextBox cancel:** `UITextBox::triggerCancel` fires the event, then drops selection
      via `UIManager::setSelected(nullptr)` (setSelected drives unfocus/deselect). Escape
      is routed from `handleEvent` (below) while an editor is focused — keyboard users are
      no longer trapped in the box.
- [x] **Multi-root pointer fall-through:** base `hitTestInHierarchy` now returns `this`
      regardless of `blocksInput`, so `UIManager::hitTest`'s fallback branch (topmost
      non-blocking hit recorded, search continues to the next root) is alive for the
      first time — it was dead because the old implementations never returned
      non-blocking widgets.
- [x] **Base `hitTestInHierarchy` recurses children** through `getChildTransform()` — the
      exact transform `drawHierarchy` applies, so hit-test ≡ draw. The three container
      overrides (VerticalBox / HorizontalBox / ScrollView) were deleted: their point math
      was literally `getChildTransform().transformPoint()`; the redundant box
      `toLocalSpace` overrides went too (byte-identical to base; ScrollView's stays — it
      adds the scroll offset).
- [x] **Disabled consumes:** invisible widgets return `nullptr` (transparent to input);
      disabled / non-interactable widgets return themselves (they block what's drawn
      underneath); `processPointer` gates dispatch on alive+interactable, so dead widgets
      fire nothing and let nothing through.
- [x] **Navigation walks the whole tree:** `selectFirst` / `findSelectableInDirection` DFS
      over all roots via new helpers `findFirstSelectable` / `collectSelectables`
      (containers — boxes and scroll views — excluded as candidates, matching the old
      flat canvas list where they were never members). New `toRootSpace()` helper maps
      nested parent-local rects (incl. scrolled content) to window pixels by inverting
      each ancestor's `getChildTransform()` — keeps the angle/distance scoring correct.
- [x] **Text input owned by UIManager:** `UIManager::handleEvent(const sf::Event&)` routes
      `TextEntered` (insert ≥32, backspace as code-point 8) and caret keys
      Left/Right/Home/End/Delete/Escape to the focused `UITextBox`; internal content is
      now `sf::String` (UTF-32 internally; `fromUtf8` at input, `U8String` → `std::string`
      at the API boundary — all three `toAnsiString()` round-trips are gone). Caret
      placement on click and caret drawing both use `sf::Text::findCharacterPos`
      (average-char-width heuristic removed). `main.cpp` loses its
      `dynamic_cast<UITextBox*>` block and the now-unused include.
- [x] `handleEvent` returns consumed-ness; `main.cpp` gates `InputSystem::onEvent` on it
      (Closed always passes; typing, caret keys and Escape never reach the game input
      layer while an editor is focused; Escape without an editor still closes the window,
      Backspace is consumed but only applied once — via `TextEntered`, not also as a key).

**Verification (recorded at completion):** build green — two known
`-Wdeprecated-declarations` warnings for `findCharacterPos` (the API the plan names; its
replacement `getShapedGlyphs()` is a heavier API — revisit if the warnings become a
problem); `ctest` 155 cases / 154 pass / the same 1 pre-existing `ParticleSystemTest`
failure; demo runs 8 s with zero errors and zero Lua lookup misses; layering gate empty.

**Exit:** build + tests green ✓. The *interactive* manual checks (gamepad/keyboard focus
**and click**, typing/Escape in a live textbox) cannot be executed headlessly in this
environment — automated coverage is deferred to Phase 6; the HUD-after-post-FX draw order
was verified in Phase 1.

---

### Phase 3 — Layout: slots + anchors — **DONE**

- [x] `UISlot` struct (anchorMin, anchorMax, pivot, offset) — new header `ui/UISlot.h`,
      carried on the child (`UIWidget::m_slot`, replacing the three loose dead members;
      accessors keep their old names, `getSlot/setSlot/getOffset/setOffset` added).
      Formula: `position = lerp(anchorMin⊙V, anchorMax⊙V, pivot) + offset − pivot⊙size`
      (size never stretches). `setPosition`/`setRect` rewrite the offset — documented
      authoring order: position first, anchors + anchor-relative offset LAST.
- [x] Implicit root panel: `UIManager::relayout(viewportSize)` recomputes every root from
      its slot (via `applySlot`, which restores the authoritative offset after
      `setPosition`'s sync); called from `addRoot` while the viewport is known and from
      the resize hook. Defaults `anchor=0,0/0,0` + `pivot=0,0` → offset = absolute
      top-left → manually positioned setups unchanged (verified: top-left widgets do not
      move on resize).
- [x] Hook: `sf::Event::Resized` in `handleEvent` → `relayout` (returns false — the game
      still receives resize events).
- [x] Boxes/ScrollView: children keep padding/spacing layout; the slot rides along but
      anchors are ignored there (documented in `ui/UISlot.h` and the `UIWidget` slot
      block).
- [x] Anchors no longer dead data — the `UIWidget.h` slot doc comment describes exactly
      *when* slots are applied (root level, on add + resize; ignored inside boxes/scroll).

**Two bugs the resize test caught (both fixed here):**
1. `UIWidget::setPosition/setRect/setSize` were **not virtual** — UIButton/UICheckbox/
   UISlider's dirty-marking setters were name-hiding that never fired through a base
   pointer, so `relayout` moved the rect (and hit-test) while cached vertices stayed at
   the old pixels. Fix: base setters virtual; `UISlider` gained real `setPosition/setRect`
   overrides (Button/Checkbox hiding setters became true overrides). Widgets that rebuild
   geometry every frame (UILabel/UIImage/boxes/ScrollView/UITextBox's `m_lastPos` guard)
   needed nothing.
2. `UIManager::draw` used `target.getDefaultView()`, which SFML builds **once at
   construction and never updates on resize** (`RenderWindow::onResize` only re-applies
   the current view) — the whole HUD was drawn through a stale 1280×720 space and
   stretched away from its hit-test rects. Fix: draw builds the view from
   `target.getSize()` every frame, so drawn pixels == window pixels == hit-test space.

**Verification (recorded at completion):** build green; `ctest` 155/154 pass / same 1
pre-existing `ParticleSystemTest` failure; layering gate empty; demo log clean. Exit
criterion verified **visually** (`xdotool` resize 1280×720 → 1100×750 + screenshots with
pixel-exact color bounding boxes): ExitBtn/ToggleShaderBtn/ShaderLabel measured at
`x=875`, full 200px width, bottom edges at 660/725 (= predicted `{W−225, H−140}` /
`{W−225, H−75}` + 25px margins); DebugLabel flush bottom-left; top-left widgets unmoved;
shader label re-aligned onto its button. **HUD corners stay pinned on resize.**

---

### Phase 4 — Lua API + demo migration — **DONE**

- [x] New `RegisterUIManager.cpp`: `UI:get(name)` **done** (concrete-type switch on
      `getType()`, guarded singleton bind). `UI:handleEvent` not exposed
      (C++-side). Per-widget usertypes kept as-is.
- [x] **`UI:createRoot(type[, name])`** (completed after Phase 5): the registry
      default-constructs the widget, `{}` keeps the default size, roots are
      appended in order; returns nil on unknown kind / missing registry /
      exhausted pool. Shares `widgetObject(sol::state_view, UIWidget*)` with
      `UI:get`; `widgetTypeIdForName(String)` maps script names
      (`button/label/image/checkbox/textbox/slider/verticalbox/horizontalbox/scrollview`)
      → `TypeTraits<T>::getTypeId()`; only `get/save/load/createRoot` are bound.
      Exercised by the Lua factory case in `UIManagerTest.cpp`.
- [x] Keep `RegisterUIWidget.cpp`'s script-event binder (widget → `ScriptComponent` callback
      is fine — the *listener* is scene-side, the widget is not).
- [x] Migrate `Game/resources/gameManager.lua`: all 14 `scene:findNode(...):getComponent(UI…)`
      lookups → `UI:get("<name>")`; `*Node` globals dropped; nil-checks/messages kept.
- [x] **Check init timing:** verified — the UI block in `main.cpp` (~L270-478) runs before
      the `gameManager` node/ScriptComponent (~L480), and `ScriptEngine::startUp` binds
      `UI` before any script runs. Runtime-confirmed: cooked asset re-cooked after the
      edit, zero "…not found" prints in a live run. No `onUIReady` hook needed.
- [x] Demo HUD: every widget gets a name at creation in `main.cpp` (parity list recorded
      in Phase 1).

**Exit:** build green, ctest green (Phase 6 suite includes the `UI:createRoot` Lua
case), gate empty, Lua-driven paths re-verified.

---

### Phase 5 — UI document serialization — **DONE**

- [x] `UIWidgetRegistry` (new `ui/UIWidgetRegistry.h`, `source/ui/UIWidgetRegistry.cpp`):
      UUID → `Function<UIWidget*(sf::Vector2f)>`; `registerWidget<T>()` keyed by
      `TypeTraits<T>::getTypeId()` (name-derived, stable), `create(typeId, size)`,
      `isRegistered`. Scene's `ComponentRegistry` untouched (stays scene-only).
- [x] `UISerializer` (new `ui/UISerializer.h`, `source/ui/UISerializer.cpp`): static
      `serialize/deserialize` (memory) + `saveToFile/loadFromFile` — container is a
      single-chunk `.sfmxasset`, `ChunkFormat::kRaw` + `ChunkCompression::kLz4`
      (pattern copied from `SceneSerializer::saveToFile/loadFromFile`).
      Record = `[type UUID][uint64 payload size][uint32 child count][payload]`,
      children depth-first; an unknown type costs only its own bytes — payload
      **and** child subtrees are consumed via `skipRecord`, nothing created.
- [x] **Uniform base-state payload:** `UIWidget::serializeBase/deserializeBase`
      (decl after `onDeserialize` in `UIWidget.h`, impl in `UIWidget.cpp`):
      `uint8` base version, name, flags bits 0–4, rect, **full slot**
      (anchorMin/anchorMax/pivot/offset — offset now round-trips too), colour.
      All 9 widget `onSerialize/onDeserialize` call it right after their own
      version byte; `deserializeBase` writes members via the (virtual) setters so
      cached-geometry widgets mark `m_visualDirty`, and re-applies `setOffset`
      last (setRect rewrites the offset datum). Fixes `UILabel`/`UIImage`
      silently losing rect/flags/anchors (never written before).
      Version bumps: UIButton 3→4, UICheckbox 2→3 (its `m_checked` left the
      shared flags byte → own byte; range check → strict), UISlider 2→3 (strict),
      UITextBox 1→2, UIVerticalBox/UIHorizontalBox/UIScrollView 1→2,
      UILabel 1→2, UIImage 1→2.
- [x] `UILabel` text made font-independent (found by the new test): UTF-32
      backing store `m_textContent` + `m_charSize` + `m_textColor`, mirroring the
      `UITextBox` Phase 2 pattern — `setText/getText` round-trip UTF-8 with no
      font present, and `setFontAsset` pushes the backing state once the font
      resolves (previously text/size/colour lived only inside `sf::Text` and were
      dropped when the font wasn't loaded — including on every document load).
- [x] Registrations: `UIManager::onStartUp` starts `UIWidgetRegistry` and
      `registerWidget<T>()` for all 9 types, beside the pool registrations
      (factories allocate through those very pools).
- [x] `UIManager::saveUI/loadUI(const String& path)` (`String` so the Lua
      binding needs no `Path` include); `loadUI` **appends** its roots
      (documented — existing roots stay), each going through `addRoot` so its
      slot is applied immediately.
- [x] Lua: `UI:save(path)` / `UI:load(path)` on the `UIManager` usertype.
- [x] `Tests/UISerializerTest.cpp` (registered in `Tests/CMakeLists.txt`) — 4
      cases: (1) full 9-type nested tree round-trip incl. base state
      (rect/flags/slot/name/colour) and per-widget state (label text, checkbox
      checked, slider value, box spacing/padding, scroll offset, textbox text,
      image texture id); (2) unknown-type record skipped, stream stays aligned,
      its subtree dropped; (3) unknown widget version → defaults for that record
      only, next record still parses; (4) `.sfmxasset` on-disk round-trip via
      `tempDirectory/sfmx_ui_test` (save → destroy → load → identical tree).

**Exit:** build green; ctest **159 cases / 158 pass / 1 pre-existing
`ParticleSystemTest` fail** (baseline was 155/154/1 — +4 new cases, all green);
layering gate empty; demo runs 12 s with zero errors.

---

### Phase 6 — Test hardening + cleanup — **DONE**

- [x] `UIHitTestTest.cpp` (**8 cases**) — containsPoint interior/exterior; box-at-origin
      control; offset-container + disabled-child consume; two-level nesting;
      ScrollView scroll-offset; blocking vs fall-through matrix; root draw-order /
      removal; disabled/invisible/non-interactable consume matrix.
- [x] `UIManagerTest.cpp` (**7 cases**) — selection lifecycle + destroy-clears;
      selectFirst / directional scoring / disabled skipped; explicit nav links +
      disabled-link fallback; nested candidates in root space; submit (button +
      checkbox via `triggerSubmit` through `UIWidget*`); textbox `handleEvent`
      consumed matrix + Escape cancel; Lua `UI:createRoot` factory.
- [x] `UILayoutTest.cpp` (**3 cases**) — `UISlot::computePosition` corners/centre/
      stretch hand-computed; root reflow via `relayout` + `Resized`→false + offset
      restoration + size kept; box auto-layout invariant under relayout (child slots
      ignored, documented behaviour).
- [x] **TDD red→green on two real bugs the tests isolated** (both algebraically
      confirmed against the draw-path ground truth `states.transform *= getChildTransform()`):
  - **Bug A** — `UIWidget::hitTestInHierarchy` mapped the point with
    `getChildTransform().transformPoint()` (wrong direction): nested children of
    non-zero containers were unhittable, *and* points outside a sibling could
    falsely descend into it. Fixed to `getInverse().transformPoint()`; the code
    comment and the `UIWidget.h` doc rewritten to state the inverse direction.
  - **Bug B** — `UIManager::toRootSpace` applied `.getInverse()` walking up
    (the mirror of A): nested nav candidates landed mirrored → `dot ≤ 0` →
    discarded, so selection never moved to nested widgets. Fixed to the forward
    `getChildTransform().transformPoint()`; the doc comment's "applies each
    parent's inverse" premise rewritten.
  Red run: **177 / 172 / 5** (4 isolating cases + 1 pre-existing) → green run:
  **177 / 176 / 1** (the 1 is the pre-existing `ParticleSystemTest.cpp:329-332`,
  broken in `2cb76be`, unrelated to UI).
- [x] `UIManager::hitTest` + `moveSelection` moved private → public (test surface;
      lifecycle helpers stay private).
- [x] Sweep for leftovers: `grep -rn 'UIEventSystem\|CanvasComponent\|s_canvasDrawing\|syncColliderToRect'`
      over FrameworkGame + Game + Tests → **empty** (only this doc mentions them,
      as the historical record it is). Stale zstd comments at
      `SceneSerializer.cpp:201` + `AssetFile.cpp` (5, 35, 44, 48, 68) reworded to
      LZ4 — text-only, `kLz4Level = 19` untouched (`kLz4Level` is inert —
      `LZ4_compress_default` takes no level; kept for provenance). Old-class
      comment references rewritten: `UIEventSystem::onShutDown/update`,
      `Canvas::hitTest`, "old flat canvas list", the class-doc trio line.
      `grep -rn '#include "scene/'` over `include/ui` + `source/ui` → **empty**.
      Note (out of UI scope, left as-is): `compressLz4`'s `written < 0` check is a
      latent bug on a `size_t` — can never fire.
- [x] README / header docs updated to the new usage pattern — README gained a
      "UI usage" section (createRoot / slot anchors / update+draw order / save+load,
      C++ and Lua); `UIManager.h` / `UIWidget.h` doc comments corrected alongside the
      two transform fixes.
- [x] `appveyor.yml` test-exit propagation — reviewed and **not changed**: CI
      infrastructure, outside the UI-only scope of this redesign.

**Exit:** full suite green (177 cases / 176 pass / 1 pre-existing Particle fail);
grep sweeps clean; demo `Build/x64/Debug/Game` (12 s, 0 errors) clean.

---

## Process notes (read before starting)

**Build & test**
- Build: `cmake --build Build --config Debug` (incremental). **Do not use
  `BuildScripts/build.sh`** — it does `rm -rf Build` (full rebuild).
- Test: `ctest --test-dir Build --output-on-failure`.
- New test files must be added to the `add_executable(UnitTests …)` list in
  `Tests/CMakeLists.txt` or they won't compile/run.

**CI lies**
- `appveyor.yml` deliberately does **not** propagate the test exit code — the badge stays
  green with failing tests. Always verify locally with `ctest`. (Optional: fix the YAML in
  Phase 6.)

**Compiler warnings**
- No `-Wall/-Wextra`, no sanitizers, no LTO in `CMakeLists.txt`. Warnings only appear if you
  read the build output — read it, especially after Phase 1's type surgery.

**Pools**
- Fixed-size pools: size generously at registration. Registration must happen **before the
  first allocation** — `"pool type used without registerPool<T>()"` on stderr = a widget
  type wasn't registered in `UIManager::onStartUp`.
- Widgets must be destroyed through the pool (`deallocate(typeId, ptr)`), never `delete`.

**Old content compatibility**
- Old cooked scenes (`Game/assets/*.sfmxasset`) contain UI components as scene components.
  After Phase 1 they're no longer registered → `SceneSerializer` skips their bytes silently.
  Scenes still load; the baked UI is simply absent (the demo builds its HUD in `main.cpp`
  at runtime, so nothing visible is lost). Re-cook via DemoCook only if you want clean files.

**Events / lifetime pitfalls**
- `Event<>` asserts in `BaseConnectionNode::~BaseConnectionNode` if connections outlive the
  emitter. Keep the existing pattern: hold `HEvent` handles for the life of the
  subscription (`toggleShaderHandle` in `main.cpp`), reset them when the widget dies.
- Teardown order in `main.cpp` matters: UIManager (roots) must shut down **before**
  `MemoryPoolHandler::shutDown`, and after scenes (so `ScriptComponent` listeners are alive
  while widgets can still fire events).

**SFML coordinate invariant**
- Hit-test uses `sf::Mouse::getPosition(window)` = window pixels; drawing must use the
  default view. `UIManager::draw` sets it itself (as `CanvasComponent::onDraw` did). Don't
  apply a camera view to UI.

**Definition of done per phase**
- Build green + `ctest` green + demo manual check. Commit per coherent step; the branch is
  shared, keep it runnable.

## Out of scope / deferred

- World-space (in-scene) widgets — decision #1, not building the bridge.
- UMG-style stretch/percent size rules — decision #3, simplest model only.
- UI draw-call batching (widgets draw individually; fine for a HUD).
- Fluent builder construction syntax — decision #6, deferred.
- Dead `ThirdParty/zstd` tree removal — unrelated, separate cleanup.
