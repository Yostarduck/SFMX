#include "scene/Component.h"
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

// Component (and the ComponentT<> CRTP helper) are header-only: their members
// are defined inline in scene/Component.h. This translation unit exists only so
// the header is compiled on its own and kept self-contained.

namespace sfmx
{


void
Component::onDraw(sf::RenderTarget& target, sf::RenderStates states) const{
    SFMX_PARAMETER_UNUSED(target);
    SFMX_PARAMETER_UNUSED(states);
}

}