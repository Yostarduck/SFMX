#include "ui/UIEventSystem.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "input/Gamepad.h"
#include "input/Keyboard.h"
#include "input/Mapping.h"
#include "input/ActionMap.h"
#include "input/InputAction.h"
#include "scene/UICanvasComponent.h"
#include "ui/UIWidget.h"

namespace sfmx
{

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

void
UIEventSystem::onShutDown() {
}

// -----------------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------------

void
UIEventSystem::init(Mapping* uiMapping) {
  ActionMap* map = uiMapping->findMap("UI");
  if (!map) return;

  if (InputAction* a = map->findAction("Submit"))
    m_submitSub = a->onStarted([this](const InputContext&) { m_submit = true; });

  if (InputAction* a = map->findAction("Cancel"))
    m_cancelSub = a->onStarted([this](const InputContext&) { m_cancel = true; });
}

// -----------------------------------------------------------------------------
// Per-frame reset
// -----------------------------------------------------------------------------

void
UIEventSystem::beginFrame() {
  m_submit = false;
  m_cancel = false;
}

// -----------------------------------------------------------------------------
// Focus
// -----------------------------------------------------------------------------

void
UIEventSystem::setFocus(UIWidget* w) {
  if (w == m_focusedWidget) return;
  if (m_focusedWidget) m_focusedWidget->setFocused(false);
  m_focusedWidget = w;
  if (w) w->setFocused(true);
}

// -----------------------------------------------------------------------------
// Navigation
// -----------------------------------------------------------------------------

void
UIEventSystem::handleNavigation() {
  // Auto-focus first selected widget on first call
  if (!m_hasEverFocused) {
    m_hasEverFocused = true;
    if (m_firstSelected)
      setFocus(m_firstSelected);
  }

  if (!Keyboard::isStarted()) return;
  Keyboard& kb = Keyboard::instance();

  int dx = 0, dy = 0;

  if (kb.wasPressedThisFrame(Key::kLeft))  dx = -1;
  if (kb.wasPressedThisFrame(Key::kRight)) dx =  1;
  if (kb.wasPressedThisFrame(Key::kUp))    dy = -1;
  if (kb.wasPressedThisFrame(Key::kDown))  dy =  1;

  if (dx == 0 && dy == 0 && Gamepad::isStarted()) {
    GamepadDevice& gp = Gamepad::instance().get(0);
    const float lx = gp.getAxis(Axis::kLeftX);
    const float ly = gp.getAxis(Axis::kLeftY);
    const float px = gp.getAxis(Axis::kPovX);
    const float py = gp.getAxis(Axis::kPovY);

    constexpr float kStickDead = 0.4f;
    if (std::fabs(lx) > kStickDead) dx = (lx > 0.f) ? 1 : -1;
    if (std::fabs(ly) > kStickDead) dy = (ly > 0.f) ? 1 : -1;
    if (dx == 0 && px != 0.f)       dx = static_cast<int>(px);
    if (dy == 0 && py != 0.f)       dy = static_cast<int>(py);
  }

  if (dx != 0 || dy != 0)
    focusDirectional(dx, dy);
}

sf::Vector2f
UIEventSystem::widgetCenter(const UIWidget& w) const {
  const sf::FloatRect r = w.getWorldRect();
  return {r.position.x + r.size.x * 0.5f, r.position.y + r.size.y * 0.5f};
}

void
UIEventSystem::focusDirectional(int dx, int dy) {
  if (!m_registeredCanvas) return;

  UIWidget* cur = m_focusedWidget;

  if (cur) {
    switch (cur->getNavigationMode()) {
      case NavigationMode::None:
        return;

      case NavigationMode::Explicit: {
        UIWidget* target = nullptr;
        if (dx < 0) target = cur->getNavLeft();
        if (dx > 0) target = cur->getNavRight();
        if (dy < 0) target = cur->getNavUp();
        if (dy > 0) target = cur->getNavDown();
        if (target) { setFocus(target); }
        return;
      }

      case NavigationMode::Horizontal:
        dy = 0;
        break;

      case NavigationMode::Vertical:
        dx = 0;
        break;

      default:
        break;
    }
  }

  // Geometric nearest-neighbor search
  Vector<UIWidget*> list;
  m_registeredCanvas->collectFocusable(list);
  if (list.size() < 2) return;
  if (!cur) { setFocus(list.front()); return; }

  const sf::Vector2f curCenter = widgetCenter(*cur);

  UIWidget* best = nullptr;
  float bestDot = -std::numeric_limits<float>::max();

  for (UIWidget* w : list) {
    if (w == cur) continue;
    const sf::Vector2f c = widgetCenter(*w);
    const sf::Vector2f delta = c - curCenter;

    if ((dx != 0 && delta.x * static_cast<float>(dx) <= 0.f) ||
        (dy != 0 && delta.y * static_cast<float>(dy) <= 0.f))
      continue;

    const float dot = delta.x * static_cast<float>(dx) + delta.y * static_cast<float>(dy);
    if (dot > bestDot) {
      bestDot = dot;
      best = w;
    }
  }

  if (best) {
    setFocus(best);
  } else if (m_navigationWrap) {
    for (UIWidget* w : list) {
      if (w == cur) continue;
      const sf::Vector2f c = widgetCenter(*w);
      const sf::Vector2f delta = c - curCenter;

      if ((dx != 0 && delta.x * static_cast<float>(dx) > 0.f) ||
          (dy != 0 && delta.y * static_cast<float>(dy) > 0.f)) {
        setFocus(w);
        break;
      }
    }
  }
}

} // namespace sfmx
