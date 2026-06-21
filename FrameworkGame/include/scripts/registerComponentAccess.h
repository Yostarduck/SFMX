#pragma once

#include <sol/sol.hpp>

#include <type_traits>

#include "scene/Component.h"
#include "scene/SceneNode.h"
#include "scene/SceneTypes.h"

namespace sfmx
{

namespace script
{

/**
 * @brief Generic, type-driven component access for SceneNode from Lua.
 *
 * SceneNode::addComponent / getComponent are C++ templates and cannot be bound
 * to Lua directly. Instead, each pooled component type registers a small set of
 * type-erased thunks here (keyed by its @ref ComponentTypeId) via
 * @ref registerComponentType. Lua scripts then pass the component's usertype
 * table - which carries a @c typeId static stamped at registration time - and
 * the dispatchers below read that id and invoke the matching thunk:
 *
 * @code{.lua}
 *   local sprite = node:addComponent(SpriteComponent)
 *   if node:hasComponent(SpriteComponent) then
 *     node:getComponent(SpriteComponent):setColor(red)
 *   end
 *   node:removeComponent(SpriteComponent)
 * @endcode
 *
 * Only pool-allocated components belong here. The inline, always-present
 * Transform is reached through @c node:transform(), not this registry.
 */

/**
 * @brief Type-erased thunk that constructs component type @p T on a node from
 *        the trailing Lua arguments of @c node:addComponent(Type, ...).
 *
 * SceneNode::addComponent is variadic at compile time, so a type with
 * constructor arguments (e.g. CameraComponent's viewport) needs a thunk that
 * reads those arguments back out of @p args and forwards them to the right
 * @c addComponent<T>(...) instantiation. Return the new component (or nil).
 */
using ComponentAddFn =
  sol::object (*)(sol::state_view, SceneNode&, const sol::variadic_args&);

namespace detail
{

/** @brief Type-erased Lua entry points for one registered component type. */
struct ComponentBinding
{
  ComponentAddFn add;
  sol::object (*get)(sol::state_view, SceneNode&);
  bool (*has)(SceneNode&);
  void (*remove)(SceneNode&);
};

/** @brief Insert or replace the binding keyed by @p typeId. Defined in the .cpp
 *         so the backing table is a single process-wide instance. */
void
setComponentBinding(const ComponentTypeId& typeId, const ComponentBinding& binding);

}  // namespace detail

/**
 * @brief Register component type @p T with a custom constructor-forwarding
 *        @p add thunk.
 *
 * Use this overload for components whose constructor takes more than the owning
 * node, so @p add can pull the extra arguments from the Lua call and pick the
 * right @c addComponent<T>(...) overload. The get/has/remove thunks are still
 * generated automatically. @p T must derive from @ref Component.
 */
template<typename T>
void
registerComponentType(ComponentAddFn add) {
  static_assert(std::is_base_of<Component, T>::value,
                "registerComponentType<T>: T must derive from Component");
  const detail::ComponentBinding binding{
    add,
    [](sol::state_view lua, SceneNode& node) -> sol::object {
      return sol::make_object(lua, node.getComponent<T>());
    },
    [](SceneNode& node) -> bool {
      return node.hasComponent<T>();
    },
    [](SceneNode& node) -> void {
      node.removeComponent<T>();
    }
  };
  detail::setComponentBinding(componentTypeId<T>(), binding);
}

/**
 * @brief Register the add/get/has/remove thunks for pooled component type @p T.
 *
 * Call once per component type during Lua setup, after its usertype has been
 * created with a matching @c typeId static (see registerSpriteComponent).
 * @p T must derive from @ref Component and be constructible from @c (SceneNode*);
 * extra Lua arguments to @c addComponent are ignored. For components that take
 * constructor arguments, use the @ref registerComponentType(ComponentAddFn)
 * overload instead.
 */
template<typename T>
void
registerComponentType() {
  registerComponentType<T>(
    [](sol::state_view lua, SceneNode& node, const sol::variadic_args&)
      -> sol::object {
      return sol::make_object(lua, node.addComponent<T>());
    });
}

/**
 * @brief Lua: @c node:addComponent(TypeTable, ...). Returns the new component,
 *        or nil if @p type is not a registered component type or its pool is
 *        full. Any trailing arguments are forwarded to the type's add thunk.
 */
NODISCARD sol::object
luaAddComponent(sol::this_state state,
                SceneNode& node,
                sol::table type,
                sol::variadic_args args);

/** @brief Lua: @c node:getComponent(TypeTable). Returns the component, or nil. */
NODISCARD sol::object
luaGetComponent(sol::this_state state, SceneNode& node, sol::table type);

/** @brief Lua: @c node:hasComponent(TypeTable). */
NODISCARD bool
luaHasComponent(SceneNode& node, sol::table type);

/** @brief Lua: @c node:removeComponent(TypeTable). No-op if absent or unknown. */
void
luaRemoveComponent(SceneNode& node, sol::table type);

}  // namespace script

}  // namespace sfmx