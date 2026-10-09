# SFMX

[![Build status](https://ci.appveyor.com/api/projects/status/xvyu1tol4pikgajs?svg=true)](https://ci.appveyor.com/project/USwampertor/sfmx)


SFMX is a extended SFML framework which purpose is to develop a game

## UI usage

The UI is a self-contained widget layer — widgets never touch `SceneNode`.
They are pool-allocated (`ui::createWidget<T>`), and `UIManager` owns the
roots: insertion order = draw order, hit-tested newest-first.

```cpp
// start-up — registers the widget pools and the widget-type registry
UIManager::startUp();

// build — a widget is EITHER a root or a child, never both
UIButton* play = ui::createWidget<UIButton>(sf::Vector2f{240.f, 64.f});
play->setPosition({520.f, 400.f});   // authored position first …
play->setAnchorMin({0.5f, 0.5f});    // … anchors + pivot after (slot rule)
play->setAnchorMax({0.5f, 0.5f});
play->setPivot({0.5f, 0.5f});        // centred, reflows on every resize
HEvent playClick = play->onPointerClick([](sf::Vector2f) { /* … */ });
UIManager::instance().addRoot(play);

// per frame — update before the scene, draw AFTER post-processing
UIManager::instance().update(window, dt);
UIManager::instance().draw(target);

// persist — versioned widget-tree format
UIManager::instance().saveUI("ui/main_menu.json");
UIManager::instance().loadUI("ui/main_menu.json");  // appends new roots
```

```lua
-- script side
local btn  = UI:createRoot('button', 'PlayBtn')  -- nil for unknown kinds
local same = UI:get('PlayBtn')                   -- recursive name lookup
UI:save('ui/main_menu.json')
```

Keyboard/gamepad navigation, scroll views and text input are all driven by the
same manager — see `docs/UI_REDESIGN.md` for the full model.
