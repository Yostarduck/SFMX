#pragma once

#include <sol/sol.hpp>

namespace sfmx
{

namespace script
{

void
registerTransformComponent(sol::state_view lua);

}  // namespace script

}  // namespace sfmx