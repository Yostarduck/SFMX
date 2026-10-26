/************************************************************************/
/**
 * @file UIFactory.h
 * @author Swampertor
 * @date 2026/06/10
 * @brief  Pool-allocated widget creation: ui::createWidget<T>(...).
 */
/************************************************************************/
#pragma once

#include "core/platform/Prerequisites.h"
#include "utils/MemoryPoolHandler.h"

namespace sfmx
{

namespace ui
{

/**
 * @brief Allocate a widget @p T from its registered memory pool.
 *
 * Pools are registered by UIManager::onStartUp, so the manager must be
 * started before the first call. The caller wires the widget into the tree
 * exactly once: UIManager::addRoot(w) for a root, or parent->addChild(w)
 * for a child — never both.
 *
 * Widgets are destroyed through the tree (parent dtor / UIManager::shutDown),
 * never with `delete`.
 *
 * @param args Constructor arguments for @p T.
 * @return The new widget, or nullptr if the pool is full.
 */
template<typename T, typename... Args>
NODISCARD T*
createWidget(Args&&... args) {
  T* widget =
    MemoryPoolHandler::instance().pool<T>().allocate(std::forward<Args>(args)...);
  SFMX_ASSERT(nullptr != widget);
  return widget;
}

} // namespace ui

} // namespace sfmx
