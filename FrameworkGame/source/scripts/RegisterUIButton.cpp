#include "scripts/RegisterUIButton.h"

#include "ui/UIButton.h"

#include "core/platform/Prerequisites.h"
#include "ui/UIWidget.h"

#include <utility>

namespace sfmx
{

namespace script
{

void
registerUIButton(sol::state_view lua) {
  lua.new_usertype<UIButton>("UIButton",
    sol::no_constructor,
    sol::base_classes, sol::bases<UIWidget, Component>(),

    "typeId", sol::var(componentTypeId<UIButton>())
  );
}

}  // namespace script

}  // namespace sfmx