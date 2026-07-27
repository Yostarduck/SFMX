#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "scene/Component.h"

namespace sfmx {

void Component::onDraw(sf::RenderTarget& target, sf::RenderStates states) const {
  (void)target;
  (void)states;
}

} // namespace sfmx
