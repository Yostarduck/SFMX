#include "scripts/RegisterUIScrollView.h"

#include "ui/UIScrollView.h"

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

namespace script
{

void
registerUIScrollView(sol::state_view lua) {
  lua.new_usertype<UIScrollView>("UIScrollView",
    sol::no_constructor,
    sol::base_classes, sol::bases<UIWidget, Component>(),

    "typeId", sol::var(componentTypeId<UIScrollView>()),

    "setScrollOffset", &UIScrollView::setScrollOffset,
    "getScrollOffset", &UIScrollView::getScrollOffset,

    "scrollBy", &UIScrollView::scrollBy,

    "setContentHeight", &UIScrollView::setContentHeight,
    "getContentHeight", &UIScrollView::getContentHeight,

    "setBackgroundColor", &UIScrollView::setBackgroundColor,
    "getBackgroundColor", &UIScrollView::getBackgroundColor,

    "clampScrollOffset", &UIScrollView::clampScrollOffset
  );
}

}  // namespace script

}  // namespace sfmx