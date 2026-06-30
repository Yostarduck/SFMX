#include "ui/Canvas.h"

namespace sfmx
{

Canvas::Canvas() {}

Canvas::~Canvas() {
  // Widgets are NOT owned by the Canvas — they are pool-allocated via the
  // scene component system or stack-allocated.  No destruction needed.
  // Clear canvas back-pointers so widgets don't attempt to unregister from
  // a destroyed Canvas during their own destruction.
  for (auto* w : m_widgets) {
    w->setCanvas(nullptr);
  }
}

void Canvas::addWidget(UIWidget* widget) {
  if (nullptr == widget) {
    return;
  }
  widget->setCanvas(this);
  m_widgets.push_back(widget);
}

bool Canvas::removeWidget(UIWidget* widget) {
  for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
    if (*it == widget) {
      m_widgets.erase(it);
      widget->setCanvas(nullptr);
      return true;
    }
  }
  return false;
}

void Canvas::clear() {
  m_widgets.clear();
}

UIWidget* Canvas::hitTest(sf::Vector2f localPoint) const {
  UIWidget* fallback = nullptr;

  // Iterate in reverse (back = drawn last = topmost).
  for (auto it = m_widgets.rbegin(); it != m_widgets.rend(); ++it) {
    UIWidget* w = *it;
    if (!w->isEnabled() || !w->isVisible() || !w->isInteractable()) {
      continue;
    }
    if (!w->containsPoint(localPoint)) {
      continue;
    }
    if (w->isBlockingInput()) {
      return w;
    }
    // Non-blocking widget: record and keep looking for a blocking one.
    if (fallback == nullptr) {
      fallback = w;
    }
  }
  return fallback;
}

void Canvas::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  states.transform *= m_transform;

  for (auto* w : m_widgets) {
    if (w->isVisible()) {
      w->onDraw(target, states);
    }
  }
}

} // namespace sfmx
