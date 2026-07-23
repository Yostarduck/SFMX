#include "scripts/RegisterUIButton.h"

#include "ui/UIButton.h"

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

namespace script
{

void
registerUIButton(sol::state_view lua) {
  lua.new_usertype<UIButton>("UIButton",
    sol::no_constructor,
    sol::base_classes, sol::bases<UIWidget, Component>(),

    "typeId", sol::var(componentTypeId<UIButton>()),

    "setNormalColor", &UIButton::setNormalColor,
    "setHoveredColor", &UIButton::setHoveredColor,
    "setPressedColor", &UIButton::setPressedColor,
    "setDisabledColor", &UIButton::setDisabledColor,
    "setFocusedColor", &UIButton::setFocusedColor,

    "getNormalColor", &UIButton::getNormalColor,
    "getHoveredColor", &UIButton::getHoveredColor,
    "getPressedColor", &UIButton::getPressedColor,
    "getFocusedColor", &UIButton::getFocusedColor,
    "getDisabledColor", &UIButton::getDisabledColor,

    "setSize", &UIButton::setSize,
    "setPosition", &UIButton::setPosition,
    "setRect", &UIButton::setRect,
    "setEnabled", &UIButton::setEnabled
  );
}

}  // namespace script

}  // namespace sfmx