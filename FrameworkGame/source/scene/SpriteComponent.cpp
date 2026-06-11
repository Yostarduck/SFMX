#include "scene/SpriteComponent.h"

#include <cmath>

#include "scene/SceneNode.h"

namespace sfmx
{

SpriteComponent::SpriteComponent(SceneNode* owner)
  : ComponentT<SpriteComponent>(owner)
{}

sf::Sprite&
SpriteComponent::activeSprite()
{
  SFMX_ASSERT(m_sprite.has_value());
  return *m_sprite;
}

const sf::Sprite&
SpriteComponent::activeSprite() const
{
  SFMX_ASSERT(m_sprite.has_value());
  return *m_sprite;
}

void
SpriteComponent::setTexture(const sf::Texture& texture)
{
  m_sprite.emplace(texture);
}

void
SpriteComponent::setRect(const sf::IntRect& rect)
{
  activeSprite().setTextureRect(rect);
}

const sf::IntRect&
SpriteComponent::getRect() const
{
  return activeSprite().getTextureRect();
}

void
SpriteComponent::setColor(const sf::Color& color)
{
  activeSprite().setColor(color);
}

sf::Color
SpriteComponent::getColor() const
{
  return activeSprite().getColor();
}

void
SpriteComponent::move(const sf::Vector2f& delta)
{
  activeSprite().move(delta);
}

void
SpriteComponent::setPosition(const sf::Vector2f& position)
{
  activeSprite().setPosition(position);
}

sf::Vector2f
SpriteComponent::getPosition() const
{
  return activeSprite().getPosition();
}

void
SpriteComponent::rotate(float deltaDegrees)
{
  activeSprite().rotate(sf::degrees(deltaDegrees));
}

void
SpriteComponent::rotate(const sf::Angle& angle)
{
  activeSprite().rotate(angle);
}

void
SpriteComponent::setRotation(const sf::Angle& angle)
{
  activeSprite().setRotation(angle);
}

void
SpriteComponent::setRotation(float degrees)
{
  activeSprite().setRotation(sf::degrees(degrees));
}

sf::Angle
SpriteComponent::getRotation() const
{
  return activeSprite().getRotation();
}

float
SpriteComponent::getRotationDegrees() const
{
  return activeSprite().getRotation().asDegrees();
}

void
SpriteComponent::scale(float delta)
{
  m_baseScale.x *= delta;
  m_baseScale.y *= delta;
  syncScale();
}

void
SpriteComponent::scale(const sf::Vector2f& delta)
{
  m_baseScale.x *= delta.x;
  m_baseScale.y *= delta.y;
  syncScale();
}

void
SpriteComponent::setScale(float newScale)
{
  m_baseScale = {newScale, newScale};
  syncScale();
}

void
SpriteComponent::setScale(const sf::Vector2f& newScale)
{
  m_baseScale = newScale;
  syncScale();
}

sf::Vector2f
SpriteComponent::getScale() const
{
  return m_baseScale;
}

void
SpriteComponent::syncScale()
{
  sf::Vector2f effective = m_baseScale;
  if (m_flipX) effective.x = -std::abs(effective.x);
  if (m_flipY) effective.y = -std::abs(effective.y);
  activeSprite().setScale(effective);
}

void
SpriteComponent::setOrigin(const sf::Vector2f origin)
{
  activeSprite().setOrigin(origin);
}

sf::Vector2f
SpriteComponent::getOrigin() const
{
  return activeSprite().getOrigin();
}

void
SpriteComponent::flipX(bool flipped)
{
  m_flipX = flipped;
  syncScale();
}

void
SpriteComponent::flipY(bool flipped)
{
  m_flipY = flipped;
  syncScale();
}

bool
SpriteComponent::isFlippedX() const
{
  return m_flipX;
}

bool
SpriteComponent::isFlippedY() const
{
  return m_flipY;
}

void
SpriteComponent::setFrame(const Frame& frame)
{
  if (frame.texture)
    m_sprite.emplace(*frame.texture, frame.framing);
  else if (frame.framing.size.x > 0 && frame.framing.size.y > 0)
    setRect(frame.framing);

  setColor(frame.color);
  flipX(frame.flippedX);
  flipY(frame.flippedY);
}

const Frame
SpriteComponent::getAsFrame() const
{
  Frame f;
  const sf::Sprite& s = activeSprite();
  f.texture = const_cast<sf::Texture*>(&s.getTexture());
  f.framing = s.getTextureRect();
  f.color = s.getColor();
  f.flippedX = m_flipX;
  f.flippedY = m_flipY;
  return f;
}

void
SpriteComponent::onUpdate(float /*deltaTime*/)
{
  if (m_followNode && m_sprite.has_value())
  {
    setPosition(m_owner->getWorldTransform().transformPoint({0.f, 0.f}));
  }
}

void
SpriteComponent::onDraw(sf::RenderTarget& target,
                        sf::RenderStates states) const
{
  if (m_sprite.has_value())
    target.draw(*m_sprite, states);
}

} // namespace sfmx
