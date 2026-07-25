#include "scripts/RegisterSpriteComponent.h"

#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Color.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/SpriteComponent.h"
#include "assets/Asset.h"
#include "assets/TextureAsset.h"
#include "utils/TypeTraits.h"

namespace sfmx
{

namespace script
{

void
registerSpriteComponent(sol::state_view lua) {
  lua.new_usertype<SpriteComponent>("SpriteComponent",
    sol::no_constructor,
    sol::base_classes, sol::bases<Component>(),

    "typeId", sol::var(componentTypeId<SpriteComponent>()),

    // Accepts the generic Asset handle a load callback delivers; binds it only if it
    // is really a TextureAsset (else no-op), so Lua never sees the concrete type.
    "setTextureAsset", [](SpriteComponent& sprite, SPtr<IAsset> asset) {
      if (asset && asset->typeId() == TypeTraits<TextureAsset>::getTypeId()) {
        sprite.setTextureAsset(std::static_pointer_cast<TextureAsset>(asset));
      }
    },
    "setTextureAssetId", &SpriteComponent::setTextureAssetId,
    "getTextureAssetId", &SpriteComponent::getTextureAssetId,

    "setRect", &SpriteComponent::setRect,
    "getRect", &SpriteComponent::getRect,

    "setColor", &SpriteComponent::setColor,
    "getColor", &SpriteComponent::getColor,

    "move", &SpriteComponent::move,
    "setPosition", &SpriteComponent::setPosition,
    "getPosition", &SpriteComponent::getPosition,

    "rotate", sol::overload(
      [](SpriteComponent& c, float deltaDegrees) {
        c.rotate(deltaDegrees);
      },
      [](SpriteComponent& c, const sf::Angle& angle) {
        c.rotate(angle);
      }
    ),
    "setRotation", sol::overload(
      [](SpriteComponent& c, const sf::Angle& angle) {
        c.setRotation(angle);
      },
      [](SpriteComponent& c, float degrees) {
        c.setRotation(degrees);
      }
    ),
    "getRotation", &SpriteComponent::getRotation,
    "getRotationDegrees", &SpriteComponent::getRotationDegrees,

    "scale", sol::overload(
      [](SpriteComponent& c, float delta) {
        c.scale(delta);
      },
      [](SpriteComponent& c, const sf::Vector2f& delta) {
        c.scale(delta);
      }
    ),
    "setScale", sol::overload(
      [](SpriteComponent& c, float newScale) {
        c.setScale(newScale);
      },
      [](SpriteComponent& c, const sf::Vector2f& newScale) {
        c.setScale(newScale);
      }
    ),
    "getScale", &SpriteComponent::getScale,

    "setOrigin", &SpriteComponent::setOrigin,
    "getOrigin", &SpriteComponent::getOrigin,

    "getPixelSize", &SpriteComponent::getPixelSize,

    "flipX", &SpriteComponent::flipX,
    "flipY", &SpriteComponent::flipY,

    "isFlippedX", &SpriteComponent::isFlippedX,
    "isFlippedY", &SpriteComponent::isFlippedY
  );
}

}  // namespace script

}  // namespace sfmx