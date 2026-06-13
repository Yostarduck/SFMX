#pragma once

#include "core/platform/Prerequisites.h"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Color.hpp>
#include "scene/Component.h"

namespace sfmx {
class Frame;

class SpriteComponent : public ComponentT<SpriteComponent> {

public:
  explicit SpriteComponent(SceneNode* owner);

  void setTexture(const sf::Texture& txt);
  void setTexture(SPtr<sf::Texture> texture);

  void setFrame(const Frame& f);
  NODISCARD const Frame getAsFrame() const;

  void setRect(const sf::IntRect& rect);

  NODISCARD const sf::IntRect& getRect() const;

  void setColor(const sf::Color& color);

  NODISCARD sf::Color getColor() const;

  void move(const sf::Vector2f& delta);
  void setPosition(const sf::Vector2f& position);

  NODISCARD sf::Vector2f getPosition() const;

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
  void scale(const sf::Vector2f& delta);
  void setScale(float newScale);
  void setScale(const sf::Vector2f& newScale);
  NODISCARD sf::Vector2f getScale() const;

  void setOrigin(const sf::Vector2f newOrigin);

  NODISCARD sf::Vector2f getOrigin() const;

  NODISCARD sf::Vector2i getPixelSize();

  void flipX(bool flipped);
  void flipY(bool flipped);

  NODISCARD bool isFlippedX() const;
  NODISCARD bool isFlippedY() const;

  /** @brief Drives auto position from the node's world transform when m_followNode is true */
  void onUpdate(float deltaTime) override;

  void onDraw(sf::RenderTarget& target,
              sf::RenderStates states) const override;

  NODISCARD sf::Sprite& activeSprite();
  NODISCARD const sf::Sprite& activeSprite() const;

private:

  SPtr<sf::Sprite>   m_sprite;
  SPtr<sf::Texture>    m_texture;
  bool                 m_flipX      = false;
  bool                 m_flipY      = false;
};

} // namespace sfmx
