#include "scripts/RegisterComponentAccess.h"

#include "core/platform/Prerequisites.h"
#include "scene/SceneNode.h"

namespace sfmx
{

namespace script
{

namespace detail
{

namespace
{

/**
 * @brief Process-wide map from component type id to its Lua thunks.
 *
 * Function-local static so it is constructed on first use, sidestepping the
 * static-initialization-order problem: registration runs well after startup.
 */
UnorderedMap<ComponentTypeId, ComponentBinding>&
bindings() {
  static UnorderedMap<ComponentTypeId, ComponentBinding> table;
  return table;
}

}  // namespace

void
setComponentBinding(const ComponentTypeId& typeId, const ComponentBinding& binding) {
  bindings()[typeId] = binding;
}

}  // namespace detail

namespace
{

/**
 * @brief Resolve the binding for the component usertype @p type passed from Lua.
 *
 * Reads the @c typeId static stamped on the usertype and looks it up. Returns
 * nullptr when @p type is not a registered component type (a plain table, or a
 * type with no pool), so callers can answer nil/false cleanly.
 */
NODISCARD const detail::ComponentBinding*
resolveBinding(const sol::table& type) {
  const sol::optional<ComponentTypeId> typeId = type["typeId"];
  if (!typeId) {
    return nullptr;
  }
  auto& table = detail::bindings();
  const auto it = table.find(*typeId);
  return (it != table.end()) ? &it->second : nullptr;
}

}  // namespace

NODISCARD sol::object
luaAddComponent(sol::this_state state,
                SceneNode& node,
                sol::table type,
                sol::variadic_args args) {
  sol::state_view lua(state);
  const detail::ComponentBinding* binding = resolveBinding(type);
  if (nullptr == binding) {
    return sol::make_object(lua, sol::lua_nil);
  }
  return binding->add(lua, node, args);
}

NODISCARD sol::object
luaGetComponent(sol::this_state state, SceneNode& node, sol::table type) {
  sol::state_view lua(state);
  const detail::ComponentBinding* binding = resolveBinding(type);
  if (nullptr == binding) {
    return sol::make_object(lua, sol::lua_nil);
  }
  return binding->get(lua, node);
}

NODISCARD bool
luaHasComponent(SceneNode& node, sol::table type) {
  const detail::ComponentBinding* binding = resolveBinding(type);
  return (nullptr != binding) && binding->has(node);
}

void
luaRemoveComponent(SceneNode& node, sol::table type) {
  const detail::ComponentBinding* binding = resolveBinding(type);
  if (nullptr != binding) {
    binding->remove(node);
  }
}

}  // namespace script

}  // namespace sfmx