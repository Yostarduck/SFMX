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

  // Automatically sync listener position/direction with owner node each frame
  void setAutoUpdate(bool autoUpdate);
  NODISCARD bool isAutoUpdate() const;

  // Manual override (only needed if autoUpdate is off)
  void setPosition(const sf::Vector3f& position);
  void setDirection(const sf::Vector3f& direction);
  void setUpVector(const sf::Vector3f& upVector);
  void setVelocity(const sf::Vector3f& velocity);
  void setGlobalVolume(float volume);
  NODISCARD float getGlobalVolume() const;

  void onUpdate(float deltaTime) override;

private:
  bool m_autoUpdate = true;
};

} // namespace sfmx