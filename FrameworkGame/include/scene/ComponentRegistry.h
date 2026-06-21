#pragma once

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "scene/SceneNode.h"
#include "scene/SceneTypes.h"
#include "utils/Module.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

/**
 * @brief Maps a component's type id (UUID) to a factory that creates it on a
 *        @ref SceneNode — the reflection seam used to rebuild a scene from disk.
 *
 * Register each serializable component type once at startup
 * (@ref registerComponent). The scene serializer (M5) then, per component, reads
 * the stored type id, calls @ref create to allocate the right component from its
 * pool, and lets @ref Component::onDeserialize fill its fields. The analogue of
 * @ref AssetCodecRegistry, but for components instead of assets.
 *
 * A @ref Module, so it is globally reachable by any serializer; mirrors
 * @ref MemoryPoolHandler.
 */
class SFMX_UTILITY_EXPORT ComponentRegistry : public Module<ComponentRegistry>
{
 public:
  /**
   * @brief Register @c T's factory, keyed by @c componentTypeId<T>().
   * @tparam T A @ref Component subtype with a @c (SceneNode*) constructor.
   */
  template<typename T>
  void
  registerComponent();

  /**
   * @brief Create a component from a RUNTIME type id on @p node.
   *
   * This is the reflection path, for code that only has a @ref ComponentTypeId as
   * data — i.e. the scene deserializer reading a type id from a file. It resolves
   * the id to the right factory, which internally calls
   * @c node->addComponent<T>(); the concrete type is unknown here, so it returns
   * the @ref Component base (the caller then drives @ref Component::onDeserialize).
   *
   * Prefer @c node->addComponent<T>() whenever the type is known at compile time
   * (all gameplay / setup code). Reach for @ref create ONLY when dispatching on a
   * runtime type id. They are not alternatives: @ref create wraps addComponent.
   *
   * @return The new component, or @c nullptr if @p typeId is unregistered,
   *         @p node is null, or the pool is exhausted.
   */
  NODISCARD Component*
  create(const ComponentTypeId& typeId, SceneNode* node) const;

  /** @brief Whether a factory is registered for @p typeId. */
  NODISCARD bool
  isRegistered(const ComponentTypeId& typeId) const;

 private:
  friend class Module<ComponentRegistry>;
  ComponentRegistry() = default;

  using Factory = Function<Component*(SceneNode*)>;
  UnorderedMap<ComponentTypeId, Factory> m_factories;
};

template<typename T>
void
ComponentRegistry::registerComponent() {
  static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
  // Captureless lambda → no heap; registration is startup-only regardless.
  m_factories[componentTypeId<T>()] =
      [](SceneNode* node) -> Component* { return node->addComponent<T>(); };
}

} // namespace sfmx
