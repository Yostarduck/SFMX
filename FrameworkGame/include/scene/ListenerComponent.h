/************************************************************************/
/**
 * @file ListenerComponent.h
 * @author Swampertor
 * @date 2026/06/10
 * @brief  Audio listener component wrapping sf::Listener.
 */
/************************************************************************/
#pragma once

#include <SFML/Audio/Listener.hpp>
#include <SFML/System/Vector3.hpp>

#include "scene/Component.h"

namespace sfmx
{

/**
 * @brief Wraps the sf::Listener namespace functions and optionally drives
 *        the listener position from the owning node's world transform.
 */
class ListenerComponent : public ComponentT<ListenerComponent>
{
public:
  /** @brief Constructs a listener component on the given node */
  explicit ListenerComponent(SceneNode* owner);

  // Auto-sync

  /** @brief When true, position is synced from the node's world transform each frame (default: true) */
  void setAutoUpdate(bool autoUpdate);
  /** @brief Whether auto-sync is active */
  NODISCARD bool isAutoUpdate() const;

  // Manual property overrides (only meaningful when autoUpdate is off)

  /** @brief Sets the listener position in 3D space */
  void setPosition(const sf::Vector3f& position);
  /** @brief Returns the current listener position */
  NODISCARD sf::Vector3f getPosition() const;
  /** @brief Sets the forward direction of the listener */
  void setDirection(const sf::Vector3f& direction);
  /** @brief Returns the current listener direction */
  NODISCARD sf::Vector3f getDirection() const;
  /** @brief Sets the up vector of the listener */
  void setUpVector(const sf::Vector3f& upVector);
  /** @brief Returns the current listener up vector */
  NODISCARD sf::Vector3f getUpVector() const;
  /** @brief Sets the velocity for Doppler effect */
  void setVelocity(const sf::Vector3f& velocity);
  /** @brief Returns the current listener velocity */
  NODISCARD sf::Vector3f getVelocity() const;
  /** @brief Sets the global volume for all spatial audio (0-100) */
  void setGlobalVolume(float volume);
  /** @brief Returns the global volume */
  NODISCARD float getGlobalVolume() const;

  /** @brief Auto-updates position from the node's world transform when m_autoUpdate is true */
  void onUpdate(float deltaTime) override;

  /** @brief Serializes the auto-update flag (the only per-component state). */
  void onSerialize(DataStream& stream) const override;
  /** @brief Restores the state written by @ref onSerialize. */
  void onDeserialize(DataStream& stream) override;

private:
  bool m_autoUpdate = true;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::ListenerComponent)
