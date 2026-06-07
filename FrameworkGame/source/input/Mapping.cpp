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

void
Mapping::evaluate(InputSystem& system, float deltaTime) {
  for (UniquePtr<ActionMap>& map : m_maps) {
    map->evaluate(system, deltaTime);
  }
}

} // namespace sfmx
