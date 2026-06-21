#include "scene/Scene.h"

#include "utils/MemoryPoolHandler.h"

#include <algorithm>
#include <cstring>

namespace sfmx
{

Scene::Scene(StringView name)
  : m_root(nullptr),
    m_nextId(1)
{
  const size_t count = std::min(name.size(), kMaxNameLength - 1);
  if (count > 0) {
    std::memcpy(m_name.data(), name.data(), count);
  }
  m_name[count] = '\0';

  const NodeId rootId = allocateId();
  m_root = MemoryPoolHandler::instance().pool<SceneNode>().allocate(
    rootId, StringView("Root"), this);
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
  SceneNode* node =
    MemoryPoolHandler::instance().pool<SceneNode>().allocate(id, name, this);
  if (nullptr == node) {
    SFMX_ASSERT(false && "node pool exhausted");
    return nullptr;
  }

  parent->appendChild(node);
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
  // Depth-first: destroy children before the node itself. Capture the next
  // sibling before recursing, since the child is about to be deallocated.
  for (SceneNode* child = node->m_firstChild; nullptr != child;) {
    SceneNode* next = child->m_nextSibling;
    destroyNodeRecursive(child);
    child = next;
  }
  node->m_firstChild = nullptr;
  node->m_lastChild = nullptr;
  
  // Return every attached component to its pool (same capture-next dance). Drop
  // any camera from the scene's draw list first so it never dangles there.
  MemoryPoolHandler& pools = MemoryPoolHandler::instance();
  for (Component* component = node->m_firstComponent; nullptr != component;) {
    Component* next = component->getNextComponent();
    if (component->getTypeId() == componentTypeId<CameraComponent>()) {
      removeCamera(static_cast<CameraComponent*>(component));
    }
    pools.deallocate(component->getTypeId(), component);
    component = next;
  }
  node->m_firstComponent = nullptr;
  node->m_lastComponent = nullptr;

  unregisterNode(node->getId());
  pools.deallocate(TypeTraits<SceneNode>::getTypeId(),
                   static_cast<void*>(node));
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
    for (SceneNode* child = node->getFirstChild();
         nullptr != child;
         child = child->getNextSibling()) {
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
  if (PhysicsSystem::isStarted()) {
    PhysicsSystem::instance().step(deltaTime);
  }
}

void
Scene::setCamera(CameraComponent* camera) {
  m_cameras.clear();
  if (nullptr != camera) {
    m_cameras.push_back(camera);
  }
}

CameraComponent*
Scene::getCamera() const {
  return m_cameras.empty() ? nullptr : m_cameras.front();
}

void
Scene::addCamera(CameraComponent* camera) {
  if (nullptr != camera) {
    m_cameras.push_back(camera);
  }
}

void
Scene::removeCamera(const CameraComponent* camera) {
  auto it = std::find(m_cameras.begin(), m_cameras.end(), camera);
  if (it != m_cameras.end()) {
    m_cameras.erase(it);
  }
}

void
Scene::clearCameras() {
  m_cameras.clear();
}

void
Scene::draw(sf::RenderTarget& target) const {
  if (nullptr == m_root) {
    return;
  }

  if (m_cameras.empty()) {
    target.setView(target.getDefaultView());
    m_root->draw(target, sf::RenderStates::Default);
    return;
  }

  // Sort by draw order so lower values render first
  auto sorted = m_cameras;
  std::sort(sorted.begin(), sorted.end(),
    [](CameraComponent* a, CameraComponent* b) {
      return a->getDrawOrder() < b->getDrawOrder();
    });

  for (CameraComponent* cam : sorted) {
    if (!cam->getOwner() || !cam->getOwner()->isEnabledInHierarchy())
      continue;
    target.setView(cam->getView());
    m_root->draw(target, sf::RenderStates::Default);
  }
}

void
Scene::clear() {
  if (nullptr == m_root) {
    return;
  }

  MemoryPoolHandler& pools = MemoryPoolHandler::instance();

  // Destroy any components attached to the root itself. Drop any camera from
  // the scene's draw list first so it never dangles there.
  for (Component* component = m_root->m_firstComponent; nullptr != component;) {
    Component* next = component->getNextComponent();
    if (component->getTypeId() == componentTypeId<CameraComponent>()) {
      removeCamera(static_cast<CameraComponent*>(component));
    }
    pools.deallocate(component->getTypeId(), component);
    component = next;
  }
  m_root->m_firstComponent = nullptr;
  m_root->m_lastComponent = nullptr;

  // Destroy every child subtree of the root. The root itself remains allocated
  // so the scene object stays valid until the pools are torn down. Capture the
  // next sibling before recursing, since the child is about to be deallocated;
  // the root's child-list ends are reset below once the list is fully dead.
  for (SceneNode* child = m_root->m_firstChild; nullptr != child;) {
    SceneNode* next = child->m_nextSibling;
    destroyNodeRecursive(child);
    child = next;
  }

  m_root->m_firstChild = nullptr;
  m_root->m_lastChild = nullptr;
  m_cameras.clear();
}

}  // namespace sfmx
