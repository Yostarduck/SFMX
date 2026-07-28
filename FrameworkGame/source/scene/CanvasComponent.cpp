#include "scene/CanvasComponent.h"
#include "ui/UIEventSystem.h"
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace sfmx
{

CanvasComponent::CanvasComponent(SceneNode* node)
  : ComponentT<CanvasComponent>(node),
    m_canvas(MakeUnique<Canvas>()) {
  UIEventSystem::instance().registerCanvas(m_canvas.get());
}

CanvasComponent::~CanvasComponent() {
  UIEventSystem::instance().unregisterCanvas(m_canvas.get());
}

void CanvasComponent::onDraw(sf::RenderTarget& target,
                             sf::RenderStates states) const {
  const sf::View prevView = target.getView();
  target.setView(target.getDefaultView());
  m_canvas->draw(target, states);
  target.setView(prevView);
}

} // namespace sfmx
