#pragma once

#include <sol/sol.hpp>

namespace sfmx
{

namespace script
{

void
registerScriptComponent(sol::state_view lua);

}  // namespace script

}  // namespace sfmx