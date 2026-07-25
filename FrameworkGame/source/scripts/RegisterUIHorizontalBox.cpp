#include "scripts/RegisterUIHorizontalBox.h"

#include "ui/UIHorizontalBox.h"

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

namespace script
{

void
registerUIHorizontalBox(sol::state_view lua) {
  lua.new_usertype<UIHorizontalBox>("UIHorizontalBox",
    sol::no_constructor,
    sol::base_classes, sol::bases<UIWidget, Component>(),

    "typeId", sol::var(componentTypeId<UIHorizontalBox>()),

    "setSpacing", &UIHorizontalBox::setSpacing,
    "getSpacing", &UIHorizontalBox::getSpacing,

    "setPadding", &UIHorizontalBox::setPadding,
    "getPadding", &UIHorizontalBox::getPadding,

    "setBoxColor", &UIHorizontalBox::setBoxColor,
    "getBoxColor", &UIHorizontalBox::getBoxColor
  );
}

}  // namespace script

}  // namespace sfmx