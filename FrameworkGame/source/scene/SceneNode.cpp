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
    m_firstChild(nullptr),
    m_lastChild(nullptr),
    m_prevSibling(nullptr),
    m_nextSibling(nullptr),
    m_scene(scene),
    m_transform(this),
    m_firstComponent(nullptr),
    m_lastComponent(nullptr)
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

size_t
SceneNode::getChildCount() const {
  size_t count = 0;
  for (const SceneNode* child = m_firstChild;
       nullptr != child;
       child = child->m_nextSibling) {
    ++count;
  }
  return count;
}

SceneNode*
SceneNode::findChild(StringView name) const {
  for (SceneNode* child = m_firstChild;
       nullptr != child;
       child = child->m_nextSibling) {
    if (name == child->getName()) {
      return child;
    }
  }
  return nullptr;
}

void
SceneNode::appendChild(SceneNode* child) {
  child->m_parent = this;
  child->m_prevSibling = m_lastChild;
  child->m_nextSibling = nullptr;
  if (nullptr != m_lastChild) {
    m_lastChild->m_nextSibling = child;
  } else {
    m_firstChild = child;
  }
  m_lastChild = child;
}

void
SceneNode::linkComponent(Component* component) {
  component->m_prevComponent = m_lastComponent;
  component->m_nextComponent = nullptr;
  if (nullptr != m_lastComponent) {
    m_lastComponent->m_nextComponent = component;
  } else {
    m_firstComponent = component;
  }
  m_lastComponent = component;
}

void
SceneNode::unlinkComponent(Component* component) {
  if (nullptr != component->m_prevComponent) {
    component->m_prevComponent->m_nextComponent = component->m_nextComponent;
  } else {
    m_firstComponent = component->m_nextComponent;
  }
  if (nullptr != component->m_nextComponent) {
    component->m_nextComponent->m_prevComponent = component->m_prevComponent;
  } else {
    m_lastComponent = component->m_prevComponent;
  }
  component->m_prevComponent = nullptr;
  component->m_nextComponent = nullptr;
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
  newParent->appendChild(this);
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

  // Splice out of the parent's doubly-linked child list, fixing up the
  // parent's first/last ends when this node sits at either edge.
  if (nullptr != m_prevSibling) {
    m_prevSibling->m_nextSibling = m_nextSibling;
  } else {
    m_parent->m_firstChild = m_nextSibling;
  }
  if (nullptr != m_nextSibling) {
    m_nextSibling->m_prevSibling = m_prevSibling;
  } else {
    m_parent->m_lastChild = m_prevSibling;
  }

  m_prevSibling = nullptr;
  m_nextSibling = nullptr;
  m_parent = nullptr;
  m_transform.markDirty();
}

void
SceneNode::update(float deltaTime) {
  if (!m_enabled) {
    return;
  }
  for (Component* component = m_firstComponent;
       nullptr != component;
       component = component->getNextComponent()) {
    component->onUpdate(deltaTime);
  }
  for (SceneNode* child = m_firstChild;
       nullptr != child;
       child = child->m_nextSibling) {
    child->update(deltaTime);
  }
}

void
SceneNode::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  if (!m_visible) {
    return;
  }
  states.transform *= m_transform.getLocalTransform();
  for (const Component* component = m_firstComponent;
       nullptr != component;
       component = component->getNextComponent()) {
    component->onDraw(target, states);
  }
  for (const SceneNode* child = m_firstChild;
       nullptr != child;
       child = child->m_nextSibling) {
    child->draw(target, states);
  }
}

}  // namespace sfmx
