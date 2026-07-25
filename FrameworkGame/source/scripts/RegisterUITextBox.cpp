#include "scripts/RegisterUITextBox.h"

#include "ui/UITextBox.h"

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

namespace script
{

void
registerUITextBox(sol::state_view lua) {
  lua.new_usertype<UITextBox>("UITextBox",
    sol::no_constructor,
    sol::base_classes, sol::bases<UIWidget, Component>(),

    "typeId", sol::var(componentTypeId<UITextBox>()),

    "setText", &UITextBox::setText,
    "getText", &UITextBox::getText,

    "setFontAssetId", &UITextBox::setFontAssetId,
    "getFontAssetId", &UITextBox::getFontAssetId,

    "setCharacterSize", &UITextBox::setCharacterSize,
    "getCharacterSize", &UITextBox::getCharacterSize,

    "setTextColor", &UITextBox::setTextColor,
    "getTextColor", &UITextBox::getTextColor,

    "setBackgroundColor", &UITextBox::setBackgroundColor,
    "setFocusedBorderColor", &UITextBox::setFocusedBorderColor,
    "getBackgroundColor", &UITextBox::getBackgroundColor,
    "getFocusedBorderColor", &UITextBox::getFocusedBorderColor,

    "setCursorPosition", &UITextBox::setCursorPosition,
    "getCursorPosition", &UITextBox::getCursorPosition,

    "setPlaceholder", &UITextBox::setPlaceholder,
    "getPlaceholder", &UITextBox::getPlaceholder,
    "setPlaceholderColor", &UITextBox::setPlaceholderColor,
    "getPlaceholderColor", &UITextBox::getPlaceholderColor,

    "isTextEditor", &UITextBox::isTextEditor,

    "insertCharacter", &UITextBox::insertCharacter,
    "deleteCharacter", &UITextBox::deleteCharacter,
    "deleteForward", &UITextBox::deleteForward
  );
}

}  // namespace script

}  // namespace sfmx