#include "scene/SceneManager.h"

#include "scene/Scene.h"
#include "scene/SceneSerializer.h"

#include <SFML/Graphics/RenderTarget.hpp>

#include <algorithm>

namespace sfmx
{

Scene*
SceneManager::createScene(StringView name) {
  const String key(name);
  if (m_scenes.find(key) != m_scenes.end()) {
    return nullptr;
  }

  UniquePtr<Scene> scene = MakeUnique<Scene>(name);
  Scene* raw = scene.get();
  m_scenes.emplace(key, std::move(scene));

  m_activeScenes.push_back(raw);
  if (nullptr == m_activeScene) {
    m_activeScene = raw;
  }
  return raw;
}

Scene*
SceneManager::loadScene(StringView name) {
  Scene* scene = findScene(name);
  if (nullptr == scene) {
    return nullptr;
  }

  if (!isActive(scene)) {
    m_activeScenes.push_back(scene);
    if (nullptr == m_activeScene) {
      m_activeScene = scene;
    }
  }
  return scene;
}

Scene*
SceneManager::loadScene(StringView name, const FileSystemPath& path) {
  Scene* scene = createScene(name);
  if (nullptr == scene) {
    return nullptr;
  }

  if (!SceneSerializer::loadFromFile(*scene, path)) {
    destroyScene(scene);
    return nullptr;
  }
  return scene;
}

bool
SceneManager::saveScene(StringView name, const FileSystemPath& path) const {
  const Scene* scene = findScene(name);
  if (nullptr == scene) {
    return false;
  }
  return SceneSerializer::saveToFile(*scene, path);
}

void
SceneManager::unloadScene(StringView name) {
  deactivate(findScene(name));
}

void
SceneManager::unloadScene(Scene* scene) {
  deactivate(scene);
}

void
SceneManager::destroyScene(StringView name) {
  destroyScene(findScene(name));
}

void
SceneManager::destroyScene(Scene* scene) {
  if (nullptr == scene) {
    return;
  }

  deactivate(scene);

  // Return pooled nodes/components before the Scene object is freed (the Scene
  // destructor only drops ids/registry, it does not touch the pools).
  scene->clear();

  m_scenes.erase(String(scene->getName()));
}

void
SceneManager::destroyAllScenes() {
  m_activeScenes.clear();
  m_activeScene = nullptr;

  for (auto& entry : m_scenes) {
    entry.second->clear();
  }
  m_scenes.clear();
}

void
SceneManager::setActiveScene(StringView name) {
  setActiveScene(findScene(name));
}

void
SceneManager::setActiveScene(Scene* scene) {
  if (nullptr == scene) {
    return;
  }

  if (!isActive(scene)) {
    m_activeScenes.push_back(scene);
  }
  m_activeScene = scene;
}

Scene*
SceneManager::findScene(StringView name) const {
  const auto it = m_scenes.find(String(name));
  return (it != m_scenes.end()) ? it->second.get() : nullptr;
}

bool
SceneManager::hasScene(StringView name) const {
  return m_scenes.find(String(name)) != m_scenes.end();
}

bool
SceneManager::isActive(const Scene* scene) const {
  return std::find(m_activeScenes.begin(), m_activeScenes.end(), scene)
         != m_activeScenes.end();
}

void
SceneManager::update(float deltaTime) {
  // NOTE: Scene::update steps the shared PhysicsSystem, so with several active
  // scenes physics advances once per scene per frame. Pre-existing behavior of
  // Scene; revisit if multiple simulated scenes are ever run together.
  for (Scene* scene : m_activeScenes) {
    scene->update(deltaTime);
  }
}

void
SceneManager::draw(sf::RenderTarget& target) const {
  for (Scene* scene : m_activeScenes) {
    scene->draw(target);
  }
}

void
SceneManager::onShutDown() {
  destroyAllScenes();
}

void
SceneManager::deactivate(Scene* scene) {
  if (nullptr == scene) {
    return;
  }

  const auto it = std::find(m_activeScenes.begin(), m_activeScenes.end(), scene);
  if (it != m_activeScenes.end()) {
    m_activeScenes.erase(it);
  }

  if (m_activeScene == scene) {
    m_activeScene = m_activeScenes.empty() ? nullptr : m_activeScenes.front();
  }
}

} // namespace sfmx
