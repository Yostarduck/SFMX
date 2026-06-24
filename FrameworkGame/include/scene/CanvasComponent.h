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
 *   auto* canvas = node.getComponent<CanvasComponent>()->getCanvas();
 *   canvas->createWidget<UIButton>("StartBtn");
 * @endcode
 */
class CanvasComponent final : public ComponentT<CanvasComponent>
{
 public:
  explicit CanvasComponent(SceneNode* node, StringView name = "Canvas");
  ~CanvasComponent() override;

  NODISCARD Canvas* getCanvas() const { return m_canvas.get(); }

 private:
  void onDraw(sf::RenderTarget& target,
              sf::RenderStates states) const override;

  std::unique_ptr<Canvas> m_canvas;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::CanvasComponent)
