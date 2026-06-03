#include "scene/SceneNode.h"

#include <algorithm>
#include <cstring>

#include "scene/Scene.h"

namespace sfmx
{

SceneNode::SceneNode(NodeId id, StringView name, Scene* scene)
  : m_id(id),
    m_enabled(true),
    m_visible(true),
    m_parent(nullptr),
    m_scene(scene),
    m_transform(this)
{
  m_name[0] = '\0';
  setName(name);
}

SceneNode::~SceneNode() = default;

void
SceneNode::setName(StringView name) {
  const size_t count = std::min(name.size(), kMaxNameLength - 1);
  if (count > 0) {
    std::memcpy(m_name.data(), name.data(), count);
  }
  m_name[count] = '\0';
}

bool
SceneNode::isEnabledInHierarchy() const {
  for (const SceneNode* node = this; nullptr != node; node = node->m_parent) {
    if (!node->m_enabled) {
      return false;
    }
  }
  return true;
}

bool
SceneNode::isAncestorOf(const SceneNode* node) const {
  for (const SceneNode* ancestor = (nullptr != node) ? node->m_parent : nullptr;
       nullptr != ancestor;
       ancestor = ancestor->m_parent) {
    if (ancestor == this) {
      return true;
    }
  }
  return false;
}

SceneNode*
SceneNode::findChild(StringView name) const {
  for (SceneNode* child : m_children) {
    if (name == child->getName()) {
      return child;
    }
  }
  return nullptr;
}

SceneNode*
SceneNode::createChild(StringView name) {
  return m_scene->createNode(name, this);
}

void
SceneNode::reparent(SceneNode* newParent, bool keepWorldTransform) {
  if (nullptr == newParent || newParent == m_parent) {
    return;
  }
  if (newParent == this || isAncestorOf(newParent)) {
    SFMX_ASSERT(false && "reparent would create a cycle");
    return;
  }

  sf::Vector2f worldOrigin;
  if (keepWorldTransform) {
    worldOrigin = getWorldTransform().transformPoint({0.f, 0.f});
  }

  detachFromParent();
  newParent->m_children.push_back(this);
  m_parent = newParent;
  m_transform.markDirty();

  if (keepWorldTransform) {
    const sf::Transform& parentWorld = newParent->getWorldTransform();
    m_transform.setPosition(parentWorld.getInverse().transformPoint(worldOrigin));
  }
}

void
SceneNode::detachFromParent() {
  if (nullptr == m_parent) {
    return;
  }
  Vector<SceneNode*>& siblings = m_parent->m_children;
  const auto it = std::find(siblings.begin(), siblings.end(), this);
  if (it != siblings.end()) {
    siblings.erase(it);
  }
  m_parent = nullptr;
  m_transform.markDirty();
}

void
SceneNode::update(float deltaTime) {
  if (!m_enabled) {
    return;
  }
  for (Component* component : m_components) {
    component->onUpdate(deltaTime);
  }
  for (SceneNode* child : m_children) {
    child->update(deltaTime);
  }
}

void
SceneNode::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!m_visible) {
    return;
  }
  states.transform *= m_transform.getLocalTransform();
  for (const Component* component : m_components) {
    component->onDraw(target, states);
  }
  for (const SceneNode* child : m_children) {
    child->draw(target, states);
  }
}

}  // namespace sfmx
