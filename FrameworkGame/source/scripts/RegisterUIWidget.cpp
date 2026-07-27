#include "scripts/RegisterUIWidget.h"

#include "ui/UIWidget.h"
#include "scene/ScriptComponent.h"
#include "scene/SceneNode.h"

#include "core/platform/Prerequisites.h"
#include "core/platform/String.h"

#include <utility>

namespace sfmx
{

namespace script
{

namespace
{

template<typename Connect>
auto scriptEventBinder(Connect connect) {
  return [connect](UIWidget& caller, ScriptComponent* target, const String& fnName) {
    if (nullptr == target) { return; }
    target->registerEvent((caller.*connect)([target, fnName](auto&&... args) {
      target->executeFunction(fnName, std::forward<decltype(args)>(args)...);
    }));
  };
}

} // namespace

void
registerUIWidget(sol::state_view lua) {
  lua.new_usertype<UIWidget>("UIWidget",
    sol::no_constructor,
    
    "setEnabled", &UIWidget::setEnabled,
    "isEnabled", &UIWidget::isEnabled,
    "setVisible", &UIWidget::setVisible,
    "setInteractable", &UIWidget::setInteractable,

    "onPointerEnter", scriptEventBinder(&UIWidget::onPointerEnter),
    "onPointerExit", scriptEventBinder(&UIWidget::onPointerExit),
    "onPointerDown", scriptEventBinder(&UIWidget::onPointerDown),
    "onPointerUp", scriptEventBinder(&UIWidget::onPointerUp),
    "onPointerClick", scriptEventBinder(&UIWidget::onPointerClick),
    "onSelect", scriptEventBinder(&UIWidget::onSelect),
    "onDeselect", scriptEventBinder(&UIWidget::onDeselect),
    "onSubmit", scriptEventBinder(&UIWidget::onSubmit),
    "onCancel", scriptEventBinder(&UIWidget::onCancel)
  );
}

}  // namespace script

}  // namespace sfmx