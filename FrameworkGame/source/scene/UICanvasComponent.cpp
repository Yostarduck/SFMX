#include "scene/UICanvasComponent.h"

#include <algorithm>
#include <limits>

#include "scene/SceneNode.h"
#include "scene/Transform.h"
#include "ui/UIEventSystem.h"
#include "ui/UIWidget.h"

#include <cmath>

#include "input/Gamepad.h"
#include "input/InputSystem.h"
#include "input/Keyboard.h"
#include "input/Mouse.h"

namespace sfmx
{

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

UICanvasComponent::UICanvasComponent(SceneNode* owner)
  : ComponentT<UICanvasComponent>(owner)
{
  setCanvasSize(m_canvasSize);
}

UICanvasComponent::~UICanvasComponent() = default;

// -----------------------------------------------------------------------------
// Canvas size
// -----------------------------------------------------------------------------

void
UICanvasComponent::setCanvasSize(sf::Vector2f size) {
  m_canvasSize = size;
  m_uiView = sf::View(sf::FloatRect({0.f, 0.f}, size));
}

// -----------------------------------------------------------------------------
// Layout
// -----------------------------------------------------------------------------

void
UICanvasComponent::resolveWidgetLayout(SceneNode* node, sf::FloatRect parentRect) {
  UIWidget* widget = nullptr;
  for (Component* c = node->getFirstComponent(); c; c = c->getNextComponent()) {
    widget = dynamic_cast<UIWidget*>(c);
    if (widget) break;
  }

  if (widget) {
    widget->resolveLayout(parentRect);
    parentRect = widget->getWorldRect();

    // Layout containers arrange their children themselves — skip them.
    if (widget->isLayoutContainer())
      return;
  }

  for (SceneNode* child = node->getFirstChild(); child; child = child->getNextSibling()) {
    resolveWidgetLayout(child, parentRect);
  }
}

void
UICanvasComponent::resolveLayout() {
  resolveWidgetLayout(m_owner, sf::FloatRect{{0.f, 0.f}, m_canvasSize});
}

// -----------------------------------------------------------------------------
// Hit-test collection
// -----------------------------------------------------------------------------

void
UICanvasComponent::collectAllHit(SceneNode* node, const sf::Vector2f& mp,
                                 Vector<UIWidget*>& outHits) const {
  for (SceneNode* child = node->getLastChild(); child; child = child->getPrevSibling()) {
    if (child->isVisible())
      collectAllHit(child, mp, outHits);
  }

  for (Component* c = node->getFirstComponent(); c; c = c->getNextComponent()) {
    UIWidget* w = dynamic_cast<UIWidget*>(c);
    if (w && w->isInteractable() && w->getWorldRect().contains(mp)) {
      outHits.push_back(w);
    }
  }
}

// -----------------------------------------------------------------------------
// Input processing (direct Mouse reads, no adapter)
// -----------------------------------------------------------------------------

void
UICanvasComponent::processInput() {
  if (!Mouse::isStarted()) return;
  Mouse& mouse = Mouse::instance();

  const sf::Vector2f mp = sf::Vector2f(mouse.getPosition());
  const bool pressedThisFrame  = mouse.wasPressedThisFrame(MouseButton::kLeft);
  const bool releasedThisFrame = mouse.wasReleasedThisFrame(MouseButton::kLeft);

  Vector<UIWidget*> hits;
  collectAllHit(m_owner, mp, hits);

  Vector<UIWidget*> activeHovered;
  activeHovered.reserve(hits.size());

  bool pressConsumed = false;
  for (UIWidget* w : hits) {
    if (pressConsumed) break;

    activeHovered.push_back(w);

    w->setHovered(true);

    if (pressedThisFrame && !pressConsumed) {
      w->setPressed(true);
      m_pressedWidgets.push_back(w);
      pressConsumed = true;
    }

    if (releasedThisFrame) {
      auto it = std::find(m_pressedWidgets.begin(), m_pressedWidgets.end(), w);
      if (it != m_pressedWidgets.end()) {
        w->setPressed(false);
        m_pressedWidgets.erase(it);
      }
    }

    if (w->isConsumingInput())
      pressConsumed = true;
  }

  // Hover exit
  for (UIWidget* prev : m_hoveredWidgets) {
    bool stillHovered = false;
    for (UIWidget* cur : activeHovered) {
      if (cur == prev) { stillHovered = true; break; }
    }
    if (!stillHovered)
      prev->setHovered(false);
  }
  m_hoveredWidgets.swap(activeHovered);

  // Release stale presses
  if (releasedThisFrame) {
    for (UIWidget* w : m_pressedWidgets)
      w->setPressed(false);
    m_pressedWidgets.clear();
  }
}

// -----------------------------------------------------------------------------
// Focus
// -----------------------------------------------------------------------------

void
UICanvasComponent::setFocus(UIWidget* w) {
  if (w == m_focusedWidget) return;
  if (m_focusedWidget) m_focusedWidget->setFocused(false);
  m_focusedWidget = w;
  if (w) w->setFocused(true);
}

void
UICanvasComponent::collectFocusable(Vector<UIWidget*>& out) const {
  struct Helper {
    static void walk(SceneNode* node, Vector<UIWidget*>& out) {
      for (SceneNode* c = node->getFirstChild(); c; c = c->getNextSibling()) {
        for (Component* comp = c->getFirstComponent(); comp; comp = comp->getNextComponent()) {
          UIWidget* w = dynamic_cast<UIWidget*>(comp);
          if (w && w->isInteractable() && w->isFocusable())
            out.push_back(w);
        }
        walk(c, out);
      }
    }
  };
  out.clear();
  Helper::walk(m_owner, out);
}

void
UICanvasComponent::focusNext() {
  Vector<UIWidget*> list;
  collectFocusable(list);
  // Remove widgets with NavigationMode::None so Tab skips them
  list.erase(std::remove_if(list.begin(), list.end(),
    [](UIWidget* w) { return w->getNavigationMode() == NavigationMode::None; }),
    list.end());
  if (list.empty()) return;

  if (!m_focusedWidget) {
    setFocus(list.front());
    return;
  }

  auto it = std::find(list.begin(), list.end(), m_focusedWidget);
  size_t idx = (it != list.end()) ? (std::distance(list.begin(), it) + 1) % list.size() : 0;
  setFocus(list[idx]);
}

void
UICanvasComponent::focusPrevious() {
  Vector<UIWidget*> list;
  collectFocusable(list);
  list.erase(std::remove_if(list.begin(), list.end(),
    [](UIWidget* w) { return w->getNavigationMode() == NavigationMode::None; }),
    list.end());
  if (list.empty()) return;

  if (!m_focusedWidget) {
    setFocus(list.back());
    return;
  }

  auto it = std::find(list.begin(), list.end(), m_focusedWidget);
  size_t idx = (it != list.end())
    ? (std::distance(list.begin(), it) + list.size() - 1) % list.size()
    : list.size() - 1;
  setFocus(list[idx]);
}

// -----------------------------------------------------------------------------
// Component hooks
// -----------------------------------------------------------------------------

void
UICanvasComponent::onUpdate(float) {
  // Auto-focus first selected widget on first update
  if (!m_hasEverFocused) {
    m_hasEverFocused = true;
    if (m_firstSelected)
      setFocus(m_firstSelected);
  }

  resolveLayout();
  processInput();

  // Set focus on click — or clear if click lands on nothing / non-focusable
  if (Mouse::isStarted() && Mouse::instance().wasPressedThisFrame(MouseButton::kLeft)) {
    if (!m_pressedWidgets.empty()) {
      UIWidget* top = m_pressedWidgets.back();
      if (top->isFocusable())
        setFocus(top);
      else
        setFocus(nullptr);
    } else {
      setFocus(nullptr);
    }
  }

  if (!Keyboard::isStarted()) return;
  Keyboard& kb = Keyboard::instance();

  // Tab / Shift+Tab from UIEventSystem
  if (UIEventSystem::isStarted()) {
    auto& ue = UIEventSystem::instance();

    if (ue.wasTabPressed()) {
      if (ue.isShiftHeld())
        focusPrevious();
      else
        focusNext();
    }

    // Submit fires click on the focused widget
    if (ue.wasSubmitted() && m_focusedWidget && m_focusedWidget->isInteractable()) {
      m_focusedWidget->setPressed(true);
      m_focusedWidget->setPressed(false);
    }
  }

  // Arrow-key and gamepad directional navigation
  {
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

  // Route text input to the focused widget (drains the buffer either way).
  Vector<char32_t> chars = InputSystem::instance().consumeTextCharacters();
  if (!chars.empty()) {
    if (m_focusedWidget)
      m_focusedWidget->onTextInput(chars);
  }
}

void
UICanvasComponent::focusDirectional(int dx, int dy) {
  // Current widget navigation mode
  UIWidget* cur = m_focusedWidget;

  // Explicit navigation
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
        dy = 0;  // constrain to x-axis
        break;

      case NavigationMode::Vertical:
        dx = 0;  // constrain to y-axis
        break;

      default:
        break;
    }
  }

  // Geometric nearest-neighbor search
  Vector<UIWidget*> list;
  collectFocusable(list);
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
    // Wrap: pick first candidate in the general direction
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

sf::Vector2f
UICanvasComponent::widgetCenter(const UIWidget& w) const {
  const sf::FloatRect r = w.getWorldRect();
  return {r.position.x + r.size.x * 0.5f, r.position.y + r.size.y * 0.5f};
}

void
UICanvasComponent::onDraw(sf::RenderTarget& target, sf::RenderStates) const {
  target.setView(m_uiView);
}

} // namespace sfmx
