#pragma once

#include "core/platform/Prerequisites.h"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Color.hpp>
#include "scene/Component.h"

namespace sfmx
{
  class Frame;

}

namespace sfmx {

class SpriteComponent : public ComponentT<SpriteComponent> {

public:

  void setTexture(const sf::Texture& txt);

  void setFrame(const Frame& f);
  NODISCARD const Frame& getAsFrame() const;

  void setRect(const sf::IntRect& rect);

  NODISCARD const sf::IntRect& getRect() const;

  void setColor(const sf::Color& color);

  NODISCARD const sf::Color& getColor() const;

  void move(const sf::Vector2f& delta);
  void setPosition(const sf::Vector2f& position);

  NODISCARD const sf::Vector2f& getPosition() const;

  /** @brief Rotates the view by an angle in degrees */
  void rotate(float deltaDegrees);
  /** @brief Rotates the view by an sf::Angle */
  void rotate(const sf::Angle& angle);
  /** @brief Sets the rotation from an sf::Angle */
  void setRotation(const sf::Angle& angle);
  /** @brief Sets the rotation in degrees */
  void setRotation(float degrees);
  /** @brief Returns the current rotation angle */
  NODISCARD sf::Angle getRotation() const;
  /** @brief Returns the current rotation in degrees */
  NODISCARD float getRotationDegrees() const;

  void scale(float delta);
  void setScale(float newScale);
  NODISCARD float getScale();


  void setOrigin(const sf::Vector2f newOrigin);

  NODISCARD const sf::Vector2f& getOrigin() const;

  void flipX(bool flipped);
  void flipY(bool flipped);

  NODISCARD bool isFlippedX();
  NODISCARD bool isFlippedY();

  /** @brief Drives auto position from the node's world transform when m_followNode is true */
  void onUpdate(float deltaTime) override;



private:

  sf::Sprite  m_sprite;


};
}