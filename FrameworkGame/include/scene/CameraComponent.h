/************************************************************************/
/**
 * @file CameraComponent.h
 * @author Swampertor
 * @date 2026/06/10
 * @brief  Camera component that wraps sf::View for the scene rendering.
 */
/************************************************************************/
#pragma once

#include <SFML/Graphics/View.hpp>

#include "core/platform/Prerequisites.h"
#include "scene/Component.h"

namespace sfmx {

/**
 * @brief Wraps an sf::View and optionally follows the owning node's world
 *        transform. The active camera is set on the render target by the
 *        Scene before the draw traversal.
 */
class CameraComponent : public ComponentT<CameraComponent>
{
 public:
  /** @brief Constructs a camera with a default view */
  CameraComponent(SceneNode* owner);
  /** @brief Constructs a camera from a viewport rectangle */
  CameraComponent(SceneNode* owner, const sf::FloatRect& viewport);
  /** @brief Constructs a camera from a center point and size */
  CameraComponent(SceneNode* owner, sf::Vector2f center, sf::Vector2f size);

  /** @brief Sets the viewport rectangle */
  void setViewport(const sf::FloatRect& viewport);
  /** @brief Returns the viewport rectangle */
  NODISCARD const sf::FloatRect& getViewport() const;

  /** @brief Access to the underlying sf::View (const) */
  NODISCARD const sf::View& getView() const;
  /** @brief Access to the underlying sf::View (mutable) */
  NODISCARD sf::View& getView();

  /** @brief Moves the center by an offset */
  void move(const sf::Vector2f& delta);
  /** @brief Sets the center point of the view */
  void setCenter(sf::Vector2f position);
  /** @brief Returns the center point of the view */
  NODISCARD sf::Vector2f getCenter() const;

  /** @brief Rotates the view by an angle in degrees */
  void rotate(float deltaDegrees);
  /** @brief Rotates the view by an sf::Angle */
  void rotate(const sf::Angle& angle);
  /** @brief Sets the rotation from an sf::Angle */
  void setRotation(const sf::Angle& angle);
  /** @brief Sets the rotation in degrees */
  void setRotation(float degrees);
  /** @brief Returns the current rotation angle */
  NODISCARD sf::Angle getRotation() const;
  /** @brief Returns the current rotation in degrees */
  NODISCARD float getRotationDegrees() const;

  /** @brief Sets the size (width/height) of the view */
  void setSize(const sf::Vector2f& newSize);
  /** @brief Returns the current size of the view */
  NODISCARD sf::Vector2f getSize() const;

  /** @brief Zooms by a factor (multiplies size by 1/factor) */
  void zoom(float factor);

  /** @brief Returns the view-to-target transform */
  NODISCARD const sf::Transform& getTransform() const;
  /** @brief Returns the inverse (target-to-view) transform */
  NODISCARD const sf::Transform& getInverseTransform() const;

  /**
   * @brief Maps a pixel coordinate (e.g. a mouse position) to world space
   *        through this camera's view, without needing an sf::RenderTarget.
   *
   * Same result as @c renderTarget.mapPixelToCoords(pixel, getView()), but the
   * caller supplies the render resolution instead of a target, so it works
   * off-thread or against a camera that isn't the one currently rendering.
   * @param pixel      Pixel position relative to the render target's top-left.
   * @param resolution Render target size in pixels (e.g. @c window.getSize()).
   */
  NODISCARD sf::Vector2f
  screenToWorld(sf::Vector2i pixel, sf::Vector2u resolution) const;

  /** @brief When true, the view center is synced from the node's world position each frame */
  void setFollowNode(bool follow);
  /** @brief Whether auto-follow is active */
  NODISCARD bool isFollowingNode() const;

  /** @brief Drives auto-follow from the node's world transform */
  void onUpdate(float deltaTime) override;

  /** @brief Draw priority (lower renders first). Default 0. */
  void setDrawOrder(uint32 order) { m_drawOrder = order; }
  /** @brief Returns the draw order. */
  NODISCARD uint32 getDrawOrder() const { return m_drawOrder; }

  /** @brief Serializes the view (center/size/rotation/viewport), follow flag and draw order. */
  void onSerialize(DataStream& stream) const override;
  /** @brief Restores the state written by @ref onSerialize. */
  void onDeserialize(DataStream& stream) override;

 private:
  sf::View m_view;
  bool     m_followNode = true;
  uint32   m_drawOrder = 0;
};

} // namespace sfmx

DECLARE_TYPE_TRAITS(sfmx::CameraComponent)
