#include "scene/Transform.h"

#include "scene/SceneNode.h"

namespace sfmx
{

Transform::Transform(SceneNode* owner)
  : ComponentT<Transform>(owner),
    m_worldDirty(true)
{}

void
Transform::setPosition(const sf::Vector2f& position) {
  m_local.setPosition(position);
  markDirty();
}

void
Transform::move(const sf::Vector2f& offset) {
  m_local.move(offset);
  markDirty();
}

void
Transform::setRotation(sf::Angle angle) {
  m_local.setRotation(angle);
  markDirty();
}

void
Transform::rotate(sf::Angle angle) {
  m_local.rotate(angle);
  markDirty();
}

void
Transform::setScale(const sf::Vector2f& factors) {
  m_local.setScale(factors);
  markDirty();
}

void
Transform::scale(const sf::Vector2f& factors) {
  m_local.scale(factors);
  markDirty();
}

const sf::Transform&
Transform::getWorldTransform() {
  if (m_worldDirty) {
    SceneNode* parent = m_owner->getParent();
    if (nullptr != parent) {
      m_worldCache = parent->transform().getWorldTransform() * m_local.getTransform();
    } else {
      m_worldCache = m_local.getTransform();
    }
    m_worldDirty = false;
  }
  return m_worldCache;
}

void
Transform::markDirty() {
  m_worldDirty = true;
  for (SceneNode* child = m_owner->getFirstChild();
       nullptr != child;
       child = child->getNextSibling()) {
    child->transform().markDirty();
  }
}

}  // namespace sfmx
