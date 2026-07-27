#include "scripts/RegisterUISlider.h"

#include "ui/UISlider.h"

#include "core/platform/Prerequisites.h"
#include "core/platform/String.h"
#include "scene/Component.h"
#include "scene/ScriptComponent.h"
#include "ui/UIWidget.h"

namespace sfmx
{

namespace script
{

void
registerUISlider(sol::state_view lua) {
  lua.new_usertype<UISlider>("UISlider",
    sol::no_constructor,
    sol::base_classes, sol::bases<UIWidget, Component>(),

    "typeId", sol::var(componentTypeId<UISlider>()),

    "getValue", &UISlider::getValue,
    "setValue", &UISlider::setValue,
    "getMinValue", &UISlider::getMinValue,
    "getMaxValue", &UISlider::getMaxValue,
    "setRange", &UISlider::setRange,

    "getNormalizedValue", &UISlider::getNormalizedValue,

    "setStepValue", &UISlider::setStepValue,
    "getStepValue", &UISlider::getStepValue,

    "setTrackColor", &UISlider::setTrackColor,
    "setFillColor", &UISlider::setFillColor,
    "setThumbColor", &UISlider::setThumbColor,
    "setThumbSize", &UISlider::setThumbSize,

    "getTrackColor", &UISlider::getTrackColor,
    "getFillColor", &UISlider::getFillColor,
    "getThumbColor", &UISlider::getThumbColor,
    "getThumbSize", &UISlider::getThumbSize,

    "setThumbTextureAssetId", &UISlider::setThumbTextureAssetId,
    "getThumbTextureAssetId", &UISlider::getThumbTextureAssetId,
    "hasThumbTexture", &UISlider::hasThumbTexture,

    "onValueChanged", [](UISlider& caller, ScriptComponent* target, const String& fnName) {
      if (nullptr == target) { return; }
      target->registerEvent(caller.onValueChanged([target, fnName](float value) {
        target->executeFunction(fnName, value);
      }));
    }
  );
}

}  // namespace script

}  // namespace sfmx