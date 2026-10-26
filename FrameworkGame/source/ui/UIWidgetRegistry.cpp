#include "ui/UIWidgetRegistry.h"

namespace sfmx
{

UIWidget*
UIWidgetRegistry::create(const UUID& typeId, sf::Vector2f size) const {
  auto it = m_factories.find(typeId);
  if (it == m_factories.end()) {
    return nullptr;
  }
  UIWidget* widget = it->second();
  // Non-zero size = explicit override; {} keeps the type's authored default.
  if (size.x > 0.f && size.y > 0.f) {
    widget->setSize(size);
  }
  return widget;
}

bool
UIWidgetRegistry::isRegistered(const UUID& typeId) const {
  return m_factories.find(typeId) != m_factories.end();
}

} // namespace sfmx
