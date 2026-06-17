#include "scene/UICanvasComponent.h"

#include <algorithm>
#include <limits>

#include "scene/SceneNode.h"
#include "scene/Transform.h"
#include "ui/UIEventSystem.h"
#include "ui/UIWidget.h"

#include <cmath>

#include "input/InputSystem.h"
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
// Focusable walk
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// Input processing (direct Mouse reads)
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

UIWidget*
UICanvasComponent::getFocus() const {
  return UIEventSystem::instance().getFocusedWidget();
}

// -----------------------------------------------------------------------------
// Component hooks
// -----------------------------------------------------------------------------

void
UICanvasComponent::onUpdate(float) {
  resolveLayout();
  processInput();

  // Set focus on click — or clear if click lands on nothing / non-focusable
  if (Mouse::isStarted() && Mouse::instance().wasPressedThisFrame(MouseButton::kLeft)) {
    if (!m_pressedWidgets.empty()) {
      UIWidget* top = m_pressedWidgets.back();
      if (top->isFocusable())
        UIEventSystem::instance().setFocus(top);
      else
        UIEventSystem::instance().setFocus(nullptr);
    } else {
      UIEventSystem::instance().setFocus(nullptr);
    }
  }

  // Navigation (arrows / gamepad)
  if (UIEventSystem::isStarted())
    UIEventSystem::instance().handleNavigation();

  // Submit fires click on the focused widget
  if (UIEventSystem::isStarted()) {
    auto& ue = UIEventSystem::instance();
    if (ue.wasSubmitted()) {
      UIWidget* focused = ue.getFocusedWidget();
      if (focused && focused->isInteractable()) {
        focused->setPressed(true);
        focused->setPressed(false);
      }
    }
  }

  // Route text input to the focused widget (drains the buffer either way).
  Vector<char32_t> chars = InputSystem::instance().consumeTextCharacters();
  if (!chars.empty()) {
    UIWidget* focused = UIEventSystem::instance().getFocusedWidget();
    if (focused)
      focused->onTextInput(chars);
  }
}

void
UICanvasComponent::onDraw(sf::RenderTarget& target, sf::RenderStates) const {
  target.setView(m_uiView);
}

} // namespace sfmx
