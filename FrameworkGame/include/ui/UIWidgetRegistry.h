/************************************************************************/
/**
 * @file UIWidgetRegistry.h
 * @author Swampertor
 * @date 2026/09/23
 * @brief  Type UUID → widget factory; rebuilds UI documents from disk.
 */
/************************************************************************/
#pragma once

#include "core/platform/Prerequisites.h"
#include "ui/UIFactory.h"
#include "ui/UIWidget.h"
#include "utils/Module.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief Maps a widget's type id (UUID) to a factory that creates it — the
 *        reflection seam @ref UISerializer uses to rebuild a UI document.
 *
 * The UI-side counterpart of @ref ComponentRegistry (which stays scene-only).
 * Each serializable widget type is registered once at startup —
 * @ref UIManager::onStartUp registers all nine built-ins; tests register what
 * they exercise. Factories allocate through @ref ui::createWidget, so the
 * widget's pool must be registered too (same place, same time).
 */
class SFMX_UTILITY_EXPORT UIWidgetRegistry : public Module<UIWidgetRegistry>
{
 public:
  /**
   * @brief Register @c T's factory, keyed by @c TypeTraits<T>::getTypeId().
   *
   * The id is name-derived and stable across runs — that is what makes a
   * document saved yesterday loadable today.
   */
  template<typename T>
  void
  registerWidget();

  /**
   * @brief Create a widget from a RUNTIME type id (the deserializer path).
   *
   * @param typeId Stored type UUID.
   * @param size   Size override applied after construction; pass @c {} to
   *               keep the type's authored default (its constructor's).
   *               The payload's setRect usually overrides either way.
   * @return The new widget — pool-allocated and NOT attached anywhere (the
   *         caller roots/parents it), or nullptr when @p typeId has no
   *         registered factory or the pool is exhausted.
   */
  NODISCARD UIWidget*
  create(const UUID& typeId, sf::Vector2f size) const;

  /** @brief Whether a factory is registered for @p typeId. */
  NODISCARD bool
  isRegistered(const UUID& typeId) const;

 private:
  friend class Module<UIWidgetRegistry>;
  UIWidgetRegistry() = default;

  using Factory = Function<UIWidget*()>;
  UnorderedMap<UUID, Factory> m_factories;
};

template<typename T>
void
UIWidgetRegistry::registerWidget() {
  static_assert(std::is_base_of_v<UIWidget, T>, "T must derive from UIWidget");
  // Captureless lambda → no heap; registration is startup-only regardless.
  // Default-constructs T (authored default size); create() may override it.
  m_factories[TypeTraits<T>::getTypeId()] =
      []() -> UIWidget* { return ui::createWidget<T>(); };
}

} // namespace sfmx
