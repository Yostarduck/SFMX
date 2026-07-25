#include "scripts/RegisterUILabel.h"

#include "ui/UILabel.h"

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

namespace script
{

void
registerUILabel(sol::state_view lua) {
  lua.new_usertype<UILabel>("UILabel",
    sol::no_constructor,
    sol::base_classes, sol::bases<UIWidget, Component>(),

    "typeId", sol::var(componentTypeId<UILabel>()),

    "setText", &UILabel::setText,
    "getText", &UILabel::getText,

    "setCharacterSize", &UILabel::setCharacterSize,
    "getCharacterSize", &UILabel::getCharacterSize,

    "setTextColor", &UILabel::setTextColor,
    "getTextColor", &UILabel::getTextColor,

    "setFontAssetId", &UILabel::setFontAssetId,
    "getFontAssetId", &UILabel::getFontAssetId
  );
}

}  // namespace script

}  // namespace sfmx