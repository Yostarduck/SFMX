#include "scene/CanvasComponent.h"
#include "ui/UIEventSystem.h"

namespace sfmx
{

CanvasComponent::CanvasComponent(SceneNode* node, StringView name)
  : ComponentT<CanvasComponent>(node),
    m_canvas(MakeUnique<Canvas>(name)) {
  UIEventSystem::instance().registerCanvas(m_canvas.get());
}

CanvasComponent::~CanvasComponent() {
  UIEventSystem::instance().unregisterCanvas(m_canvas.get());
}

void CanvasComponent::onDraw(sf::RenderTarget& target,
                             sf::RenderStates states) const {
  m_canvas->draw(target, states);
}

} // namespace sfmx
