#include "scripts/RegisterUICheckbox.h"

#include "ui/UICheckbox.h"

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "scene/ScriptComponent.h"
#include "ui/UIWidget.h"

namespace sfmx
{

namespace script
{

void
registerUICheckbox(sol::state_view lua) {
  lua.new_usertype<UICheckbox>("UICheckbox",
    sol::no_constructor,
    sol::base_classes, sol::bases<UIWidget, Component>(),

    "typeId", sol::var(componentTypeId<UICheckbox>()),

    "isChecked", &UICheckbox::isChecked,
    "setChecked", &UICheckbox::setChecked,

    "onValueChanged", [](UICheckbox& caller, ScriptComponent* target, const String& fnName) {
      if (nullptr == target) { return; }
      target->registerEvent(caller.onValueChanged([target, fnName](bool checked) {
        target->executeFunction(fnName, checked);
      }));
    },
    
    "setBoxColor", &UICheckbox::setBoxColor,
    "setCheckColor", &UICheckbox::setCheckColor,
    "setHoveredBoxColor", &UICheckbox::setHoveredBoxColor,
    "setCheckedBoxColor", &UICheckbox::setCheckedBoxColor,

    "getBoxColor", &UICheckbox::getBoxColor,
    "getCheckColor", &UICheckbox::getCheckColor,
    "getHoveredBoxColor", &UICheckbox::getHoveredBoxColor,
    "getCheckedBoxColor", &UICheckbox::getCheckedBoxColor,

    "setTextureAssetId", &UICheckbox::setTextureAssetId,
    "getTextureAssetId", &UICheckbox::getTextureAssetId,
    "hasTexture", &UICheckbox::hasTexture,

    "setSize", &UICheckbox::setSize,
    "setPosition", &UICheckbox::setPosition,
    "setRect", &UICheckbox::setRect,
    "setEnabled", &UICheckbox::setEnabled
  );
}

}  // namespace script

}  // namespace sfmx