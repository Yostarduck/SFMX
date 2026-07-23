#include "scripts/RegisterUIImage.h"

#include "ui/UIImage.h"

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"
#include "ui/UIWidget.h"

namespace sfmx
{

namespace script
{

void
registerUIImage(sol::state_view lua) {
  lua.new_usertype<UIImage>("UIImage",
    sol::no_constructor,
    sol::base_classes, sol::bases<UIWidget, Component>(),

    "typeId", sol::var(componentTypeId<UIImage>()),

    "setTextureAssetId", &UIImage::setTextureAssetId,
    "getTextureAssetId", &UIImage::getTextureAssetId,

    "setTextureRect", &UIImage::setTextureRect,
    "getTextureRect", &UIImage::getTextureRect,
    "hasTexture", &UIImage::hasTexture,

    "setFlipX", &UIImage::setFlipX,
    "setFlipY", &UIImage::setFlipY,
    "isFlippedX", &UIImage::isFlippedX,
    "isFlippedY", &UIImage::isFlippedY
  );
}

}  // namespace script

}  // namespace sfmx