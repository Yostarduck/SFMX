#include "scripts/RegisterSpriteComponent.h"

#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Color.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/SpriteComponent.h"

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

    // Texture binding by asset id; AssetManager resolves it and it round-trips
    // through serialization (see SpriteComponent::m_textureAssetId).
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