#include "input/Mapping.h"

namespace sfmx
{

Mapping::Mapping(StringView name)
  : m_name(name) {}

ActionMap*
Mapping::addMap(StringView name) {
  m_maps.push_back(MakeUnique<ActionMap>(name));
  return m_maps.back().get();
}

ActionMap*
Mapping::findMap(StringView name) const {
  for (const UniquePtr<ActionMap>& map : m_maps) {
    if (map->getName() == name) {
      return map.get();
    }
  }
  return nullptr;
}

bool
Mapping::removeMap(StringView name) {
  for (auto it = m_maps.begin(); it != m_maps.end(); ++it) {
    if ((*it)->getName() == name) {
      m_maps.erase(it);
      return true;
    }
  }
  return false;
}

void
Mapping::evaluate(InputSystem& system, float deltaTime) {
  for (UniquePtr<ActionMap>& map : m_maps) {
    map->evaluate(system, deltaTime);
  }
}

} // namespace sfmx
