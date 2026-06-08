#include "scene/ListenerComponent.h"

#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector3.hpp>

#include "scene/SceneNode.h"
#include "scene/Transform.h"

namespace sfmx
{

ListenerComponent::ListenerComponent(SceneNode* owner)
  : ComponentT<ListenerComponent>(owner)
{
}

// -----------------------------------------------------------------------------
// Auto-update
// -----------------------------------------------------------------------------

void
ListenerComponent::setAutoUpdate(bool autoUpdate) {
  m_autoUpdate = autoUpdate;
}

bool
ListenerComponent::isAutoUpdate() const {
  return m_autoUpdate;
}

// -----------------------------------------------------------------------------
// Manual property overrides
// -----------------------------------------------------------------------------

void
ListenerComponent::setPosition(const sf::Vector3f& position) {
  sf::Listener::setPosition(position);
}

sf::Vector3f
ListenerComponent::getPosition() const {
  return sf::Listener::getPosition();
}

void
ListenerComponent::setDirection(const sf::Vector3f& direction) {
  sf::Listener::setDirection(direction);
}

sf::Vector3f
ListenerComponent::getDirection() const {
  return sf::Listener::getDirection();
}

void
ListenerComponent::setUpVector(const sf::Vector3f& upVector) {
  sf::Listener::setUpVector(upVector);
}

sf::Vector3f
ListenerComponent::getUpVector() const {
  return sf::Listener::getUpVector();
}

void
ListenerComponent::setVelocity(const sf::Vector3f& velocity) {
  sf::Listener::setVelocity(velocity);
}

sf::Vector3f
ListenerComponent::getVelocity() const {
  return sf::Listener::getVelocity();
}

void
ListenerComponent::setGlobalVolume(float volume) {
  sf::Listener::setGlobalVolume(volume);
}

float
ListenerComponent::getGlobalVolume() const {
  return sf::Listener::getGlobalVolume();
}

// -----------------------------------------------------------------------------
// Component hook
// -----------------------------------------------------------------------------

void
ListenerComponent::onUpdate(float deltaTime) {
  SFMX_PARAMETER_UNUSED(deltaTime);
  if (!m_autoUpdate)
    return;

  const sf::Vector2f worldPos =
      m_owner->getWorldTransform().transformPoint({0.f, 0.f});
  sf::Listener::setPosition({worldPos.x, worldPos.y, 0.f});
}

} // namespace sfmx
