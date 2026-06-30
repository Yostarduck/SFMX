#pragma once

#include "scene/Component.h"
#include "scene/SceneNode.h"
#include "ui/Canvas.h"

namespace sfmx
{

/**
 * @brief A SceneNode component that wraps a Canvas and auto-registers it with
 *        the UIEventSystem.
 *
 * Ownership:
 *   The Canvas is owned by this component (heap-allocated unique_ptr).  When
 *   the component is destroyed (node removed), the Canvas is automatically
 *   unregistered from UIEventSystem and destroyed.
 *
 * Usage:
 * @code
 *   node.addComponent<CanvasComponent>("HUD");
 *   auto& canvas = node.getComponent<CanvasComponent>()->getCanvas();
 *   auto* btnNode = node.createChild("StartBtn");
 *   auto* btn = btnNode->addComponent<UIButton>(btnNode, "StartBtn", {200.f, 50.f});
 *   canvas.addWidget(btn);
 * @endcode
 */
class CanvasComponent final : public ComponentT<CanvasComponent>
{
 public:
  CanvasComponent(SceneNode* node);
  ~CanvasComponent() override;

  NODISCARD FORCEINLINE Canvas& getCanvas() const { return *m_canvas; }

 private:
  void onDraw(sf::RenderTarget& target,
              sf::RenderStates states) const override;

  UniquePtr<Canvas> m_canvas;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::CanvasComponent)
