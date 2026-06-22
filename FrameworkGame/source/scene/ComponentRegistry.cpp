#include "scene/ComponentRegistry.h"

#include <iostream>

namespace sfmx
{

Component*
ComponentRegistry::create(const ComponentTypeId& typeId, SceneNode* node) const {
  if (nullptr == node) {
    return nullptr;
  }

  const auto it = m_factories.find(typeId);
  if (it == m_factories.end()) {
    std::cerr << "ComponentRegistry::create: no factory for component type "
              << typeId.toString() << std::endl;
    return nullptr;
  }

  return it->second(node);
}

bool
ComponentRegistry::isRegistered(const ComponentTypeId& typeId) const {
  return m_factories.find(typeId) != m_factories.end();
}

} // namespace sfmx
