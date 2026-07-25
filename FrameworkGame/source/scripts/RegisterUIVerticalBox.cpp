#include "scripts/RegisterUIVerticalBox.h"

#include "ui/UIVerticalBox.h"

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

namespace script
{

void
registerUIVerticalBox(sol::state_view lua) {
  lua.new_usertype<UIVerticalBox>("UIVerticalBox",
    sol::no_constructor,
    sol::base_classes, sol::bases<UIWidget, Component>(),

    "typeId", sol::var(componentTypeId<UIVerticalBox>()),

    "setSpacing", &UIVerticalBox::setSpacing,
    "getSpacing", &UIVerticalBox::getSpacing,

    "setPadding", &UIVerticalBox::setPadding,
    "getPadding", &UIVerticalBox::getPadding,

    "setBoxColor", &UIVerticalBox::setBoxColor,
    "getBoxColor", &UIVerticalBox::getBoxColor
  );
}

}  // namespace script

}  // namespace sfmx