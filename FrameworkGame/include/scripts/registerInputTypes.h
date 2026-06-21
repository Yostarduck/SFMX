#pragma once

#include <sol/sol.hpp>

namespace sfmx
{

namespace script
{

void
registerInputTypes(sol::state_view lua);

}  // namespace script

}  // namespace sfmx