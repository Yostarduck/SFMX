#include "scene/Scene.h"

#include <algorithm>
#include <cstring>

namespace sfmx
{

Scene::Scene(StringView name, size_t maxNodes)
  : m_defaultComponentCapacity(64),
    m_root(nullptr),
    m_nextId(1)
{
  const size_t count = std::min(name.size(), kMaxNameLength - 1);
  if (count > 0) {
    std::memcpy(m_name.data(), name.data(), count);
  }
  m_name[count] = '\0';

  m_nodePool.initialize(maxNodes);

  const NodeId rootId = allocateId();
  m_root = m_nodePool.allocate(rootId, StringView("Root"), this);
  SFMX_ASSERT(nullptr != m_root && "node pool must hold at least the root");
  if (nullptr != m_root) {
    registerNode(m_root);
  }
}

Scene::~Scene() = default;

SceneNode*
Scene::createNode(StringView name, SceneNode* parent) {
  if (nullptr == parent) {
    parent = m_root;
  }

  const NodeId id = allocateId();
  SceneNode* node = m_nodePool.allocate(id, name, this);
  if (nullptr == node) {
    SFMX_ASSERT(false && "node pool exhausted");
    return nullptr;
  }

  node->m_parent = parent;
  parent->m_children.push_back(node);
  registerNode(node);
  return node;
}

void
Scene::destroyNode(SceneNode* node) {
  if (nullptr == node) {
    return;
  }
  SFMX_ASSERT(node != m_root && "the root cannot be destroyed");
  if (node == m_root) {
    return;
  }

  node->detachFromParent();
  destroyNodeRecursive(node);
}

void
Scene::destroyNode(NodeId id) {
  destroyNode(findNode(id));
}

void
Scene::destroyNodeRecursive(SceneNode* node) {
  // Depth-first: destroy children before the node itself.
  for (SceneNode* child : node->m_children) {
    destroyNodeRecursive(child);
  }
  node->m_children.clear();

  // Return every attached component to its pool.
  for (Component* component : node->m_components) {
    const auto it = m_componentPools.find(component->getTypeId());
    if (it != m_componentPools.end()) {
      it->second->deallocate(component);
    }
  }
  node->m_components.clear();

  unregisterNode(node->getId());
  m_nodePool.deallocate(node);
}

SceneNode*
Scene::findNode(NodeId id) const {
  const auto it = m_registry.find(id);
  return (it != m_registry.end()) ? it->second : nullptr;
}

Vector<SceneNode*>
Scene::findNodesByName(StringView name) const {
  Vector<SceneNode*> result;
  Vector<SceneNode*> stack;
  if (nullptr != m_root) {
    stack.push_back(m_root);
  }
  while (!stack.empty()) {
    SceneNode* node = stack.back();
    stack.pop_back();
    if (name == node->getName()) {
      result.push_back(node);
    }
    for (SceneNode* child : node->getChildren()) {
      stack.push_back(child);
    }
  }
  return result;
}

void
Scene::registerNode(SceneNode* node) {
  m_registry[node->getId()] = node;
}

void
Scene::unregisterNode(NodeId id) {
  m_registry.erase(id);
}

void
Scene::update(float deltaTime) {
  if (nullptr != m_root) {
    m_root->update(deltaTime);
  }
}

void
Scene::draw(sf::RenderTarget& target) const {
  if (nullptr != m_root) {
    m_root->draw(target, sf::RenderStates::Default);
  }
}

}  // namespace sfmx
