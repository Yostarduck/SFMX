#include "scene/CameraComponent.h"
#include "scene/SceneNode.h"
#include "scene/Transform.h"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Angle.hpp>

#include "core/DataStream.h"

namespace sfmx {

namespace {
/** @brief CameraComponent blob layout version; bump on format changes. */
constexpr uint32 kCameraComponentVersion = 1;
} // namespace

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

void
CameraComponent::onSerialize(DataStream& stream) const {
  stream << kCameraComponentVersion;

  const sf::Vector2f center = m_view.getCenter();
  const sf::Vector2f size   = m_view.getSize();
  const sf::FloatRect vp    = m_view.getViewport();
  stream << center.x << center.y;
  stream << size.x << size.y;
  stream << m_view.getRotation().asRadians();
  stream << vp.position.x << vp.position.y << vp.size.x << vp.size.y;

  stream << static_cast<uint8>(m_followNode ? 1 : 0);
  stream << m_drawOrder;
}

void
CameraComponent::onDeserialize(DataStream& stream) {
  uint32 version = 0;
  stream >> version;
  if (version != kCameraComponentVersion) {
    return;  // unknown version: leave defaults rather than misread bytes
  }

  float cx = 0.f;
  float cy = 0.f;
  float sx = 0.f;
  float sy = 0.f;
  float rot = 0.f;
  float vpx = 0.f;
  float vpy = 0.f;
  float vpw = 1.f;
  float vph = 1.f;
  stream >> cx >> cy >> sx >> sy >> rot >> vpx >> vpy >> vpw >> vph;

  uint8 follow = 1;
  stream >> follow;
  stream >> m_drawOrder;

  m_view.setCenter({cx, cy});
  m_view.setSize({sx, sy});
  m_view.setRotation(sf::radians(rot));
  m_view.setViewport(sf::FloatRect({vpx, vpy}, {vpw, vph}));
  m_followNode = follow != 0;
}

} // namespace sfmx
