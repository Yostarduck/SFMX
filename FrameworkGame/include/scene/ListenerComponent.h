#pragma once

#include <SFML/Audio/Listener.hpp>
#include <SFML/System/Vector3.hpp>

#include "scene/Component.h"

namespace sfmx
{

class ListenerComponent : public ComponentT<ListenerComponent>
{
public:
  explicit ListenerComponent(SceneNode* owner);

  // Auto-sync: when enabled, position/direction are driven by the node's
  // world transform each frame. Default: true.
  void setAutoUpdate(bool autoUpdate);
  NODISCARD bool isAutoUpdate() const;

  // Manual listener property overrides (only meaningful when autoUpdate is off)
  void setPosition(const sf::Vector3f& position);
  NODISCARD sf::Vector3f getPosition() const;
  void setDirection(const sf::Vector3f& direction);
  NODISCARD sf::Vector3f getDirection() const;
  void setUpVector(const sf::Vector3f& upVector);
  NODISCARD sf::Vector3f getUpVector() const;
  void setVelocity(const sf::Vector3f& velocity);
  NODISCARD sf::Vector3f getVelocity() const;
  void setGlobalVolume(float volume);
  NODISCARD float getGlobalVolume() const;

  void onUpdate(float deltaTime) override;

private:
  bool m_autoUpdate = true;
};

} // namespace sfmx
