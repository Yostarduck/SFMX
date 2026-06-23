#pragma once

#include <sol/sol.hpp>

namespace sfmx
{

namespace script
{

void
registerParticleSystemComponent(sol::state_view lua);

}  // namespace script

}  // namespace sfmx