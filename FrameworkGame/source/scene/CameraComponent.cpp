#include "scene/CameraComponent.h"
#include "scene/SceneNode.h"
#include "scene/Transform.h"

namespace sfmx {

CameraComponent::CameraComponent(SceneNode* owner)
  : ComponentT<CameraComponent>(owner)
{}

CameraComponent::CameraComponent(SceneNode* owner,
                                 const sf::FloatRect& viewport)
  : ComponentT<CameraComponent>(owner),
    m_view(viewport)
{}

CameraComponent::CameraComponent(SceneNode* owner,
                                 sf::Vector2f center,
                                 sf::Vector2f size)
  : ComponentT<CameraComponent>(owner),
    m_view(center, size)
{}

void
CameraComponent::onUpdate(float deltaTime) {
  SFMX_PARAMETER_UNUSED(deltaTime);
  if (!m_followNode)
    return;
  const sf::Vector2f worldPos =
      m_owner->transform().getWorldTransform().transformPoint({0,0});
  m_view.setCenter({worldPos.x, worldPos.y});
}

void
CameraComponent::setViewport(const sf::FloatRect& viewport) {
  m_view.setViewport(viewport);
}

const sf::FloatRect&
CameraComponent::getViewport() const {
  return m_view.getViewport();
}

const sf::View&
CameraComponent::getView() const {
  return m_view;
}

sf::View&
CameraComponent::getView() {
  return m_view;
}

void
CameraComponent::move(const sf::Vector2f& delta) {
  m_view.move(delta);
}

void
CameraComponent::setCenter(sf::Vector2f position) {
  m_view.setCenter(position);
}

sf::Vector2f
CameraComponent::getCenter() const {
  return m_view.getCenter();
}

void
CameraComponent::rotate(float deltaDegrees) {
  m_view.rotate(sf::degrees(deltaDegrees));
}

void
CameraComponent::rotate(const sf::Angle& angle) {
  m_view.rotate(angle);
}

void
CameraComponent::setRotation(const sf::Angle& angle) {
  m_view.setRotation(angle);
}

void
CameraComponent::setRotation(float degrees) {
  m_view.setRotation(sf::degrees(degrees));
}

sf::Angle
CameraComponent::getRotation() const {
  return m_view.getRotation();
}

float
CameraComponent::getRotationDegrees() const {
  return m_view.getRotation().asDegrees();
}

void
CameraComponent::setSize(const sf::Vector2f& newSize) {
  m_view.setSize(newSize);
}

sf::Vector2f
CameraComponent::getSize() const {
  return m_view.getSize();
}

void
CameraComponent::zoom(float factor) {
  m_view.zoom(factor);
}

const sf::Transform&
CameraComponent::getTransform() const {
  return m_view.getTransform();
}

const sf::Transform&
CameraComponent::getInverseTransform() const {
  return m_view.getInverseTransform();
}

void
CameraComponent::setFollowNode(bool follow) {
  m_followNode = follow;
}

bool
CameraComponent::isFollowingNode() const {
  return m_followNode;
}

} // namespace sfmx
