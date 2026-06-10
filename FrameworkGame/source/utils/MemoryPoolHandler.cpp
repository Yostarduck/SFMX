#include "utils/MemoryPoolHandler.h"

namespace sfmx
{

void
MemoryPoolHandler::deallocate(const UUID& typeId, void* object) {
  const auto it = m_pools.find(typeId);
  if (it != m_pools.end()) {
    it->second->deallocate(object);
  }
}

void
MemoryPoolHandler::reset() {
  for (auto& entry : m_pools) {
    entry.second->clear();
  }
}

size_t
MemoryPoolHandler::getTotalMemoryUsage() const {
  size_t total = 0;
  for (const auto& entry : m_pools) {
    total += entry.second->getMemoryUsage();
  }
  return total;
}

void
MemoryPoolHandler::onShutDown() {
  m_pools.clear();
}

}  // namespace sfmx
