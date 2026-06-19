/************************************************************************/
/**
 * @file Collider.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  ICollider shape hierarchy and unified intersection dispatch.
 */
/************************************************************************/
#pragma once

#include "core/platform/Prerequisites.h"
#include <SFML/Graphics/Transform.hpp>
#include <SFML/System/Vector2.hpp>

namespace sfmx
{

/** @brief Discriminator for the concrete collider type under ICollider*
 *         AABB = Axis-Aligned Bounding Box, OBB = Oriented Bounding Box */
enum class ColliderType : uint8
{
  kCircle,
  kAABB,
  kOBB,
  kPoint,
  kLine
};

// Collision result

/** @brief Result of a narrow-phase intersection test */
struct CollisionResult
{
  bool         hit         = false;
  sf::Vector2f normal;
  float        penetration = 0.f;
  NODISCARD FORCEINLINE explicit operator bool() const { return hit; }
};

// ICollider hierarchy

/**
 * @brief Abstract base for all collider shapes.
 *
 * Subclasses carry local-space data.  The free function intersect()
 * accepts any two ICollider subtypes together with their world transforms.
 */
class ICollider
{
 public:
  virtual ~ICollider() = default;
  /** @brief Returns the concrete shape type for dispatch */
  NODISCARD virtual ColliderType getType() const = 0;
};

/** @brief Circle defined by a center point and radius */
class CircleCollider : public ICollider
{
 public:
  CircleCollider() = default;
  /** @brief Construct with local-space center and radius */
  CircleCollider(const sf::Vector2f& center, float radius)
    : m_center(center), m_radius(radius) {}

  NODISCARD  FORCEINLINE ColliderType getType() const override { return ColliderType::kCircle; }

  /** @brief Set the local-space center position */
  FORCEINLINE void setCenter(const sf::Vector2f& c) { m_center = c; }
  /** @brief Set the circle radius */
  FORCEINLINE void setRadius(float r)        { m_radius = r; }
  /** @brief Local-space center position */
  NODISCARD FORCEINLINE 
  sf::Vector2f getCenter() const { return m_center; }
  /** @brief The circle radius */
  NODISCARD FORCEINLINE float 
  getRadius()        const { return m_radius; }

 private:
  sf::Vector2f m_center = {0.f, 0.f};
  float        m_radius = 16.f;
};

/** @brief Axis-aligned bounding box with center + half-size */
class AABBCollider : public ICollider
{
 public:
  AABBCollider() = default;
  /** @brief Construct with local-space center and half-extents */
  AABBCollider(const sf::Vector2f& center, const sf::Vector2f& halfSize)
    : m_center(center), m_halfSize(halfSize) {}

  NODISCARD FORCEINLINE ColliderType getType() const override { return ColliderType::kAABB; }

  /** @brief Set the local-space center position */
  FORCEINLINE void setCenter(const sf::Vector2f& c)   { m_center = c; }
  /** @brief Set the half-size extents */
  FORCEINLINE void setHalfSize(const sf::Vector2f& h) { m_halfSize = h; }
  /** @brief Local-space center position */
  NODISCARD FORCEINLINE sf::Vector2f getCenter()   const { return m_center; }
  /** @brief Half-size extents */
  NODISCARD FORCEINLINE sf::Vector2f getHalfSize() const { return m_halfSize; }

 private:
  sf::Vector2f m_center   = {0.f, 0.f};
  sf::Vector2f m_halfSize = {16.f, 16.f};
};

/** @brief Oriented bounding box (AABB rotated via its world transform) */
class OBBCollider : public ICollider
{
 public:
  OBBCollider() = default;
  /** @brief Construct with local-space center and half-extents */
  OBBCollider(const sf::Vector2f& center, const sf::Vector2f& halfSize)
    : m_center(center), m_halfSize(halfSize) {}

  NODISCARD FORCEINLINE ColliderType getType() const override { return ColliderType::kOBB; }

  /** @brief Set the local-space center position */
  FORCEINLINE void setCenter(const sf::Vector2f& c)   { m_center = c; }
  /** @brief Set the half-size extents */
  FORCEINLINE void setHalfSize(const sf::Vector2f& h) { m_halfSize = h; }
  /** @brief Local-space center position */
  NODISCARD FORCEINLINE sf::Vector2f getCenter()   const { return m_center; }
  /** @brief Half-size extents */
  NODISCARD FORCEINLINE sf::Vector2f getHalfSize() const { return m_halfSize; }

 private:
  sf::Vector2f m_center   = {0.f, 0.f};
  sf::Vector2f m_halfSize = {16.f, 16.f};
};

/** @brief Zero-area point collider */
class PointCollider : public ICollider
{
 public:
  PointCollider() = default;
  /** @brief Construct at a given local-space point */
  FORCEINLINE explicit PointCollider(const sf::Vector2f& point) : m_point(point) {}

  NODISCARD FORCEINLINE ColliderType getType() const override { return ColliderType::kPoint; }

  /** @brief Set the local-space point position */
  FORCEINLINE void setPoint(sf::Vector2f p) { m_point = p; }
  /** @brief The local-space point position */
  NODISCARD FORCEINLINE sf::Vector2f getPoint() const { return m_point; }

 private:
  sf::Vector2f m_point = {0.f, 0.f};
};

/** @brief Line segment collider (start / end) */
class LineCollider : public ICollider
{
 public:
  LineCollider() = default;
  /** @brief Construct with local-space start and end points */
  LineCollider(const sf::Vector2f& start, const sf::Vector2f& end)
    : m_start(start), m_end(end) {}

  NODISCARD FORCEINLINE ColliderType getType() const override { return ColliderType::kLine; }

  /** @brief Set the line segment start point */
  FORCEINLINE void setStart(const sf::Vector2f& p) { m_start = p; }
  /** @brief Set the line segment end point */
  FORCEINLINE void setEnd(const sf::Vector2f& p)   { m_end = p; }
  /** @brief The line segment start point */
  NODISCARD FORCEINLINE sf::Vector2f getStart() const { return m_start; }
  /** @brief The line segment end point */
  NODISCARD FORCEINLINE sf::Vector2f getEnd()   const { return m_end; }

 private:
  sf::Vector2f m_start = {-8.f, 0.f};
  sf::Vector2f m_end   = { 8.f, 0.f};
};

// Public dispatch

/** @brief Single entry point for all collider pair intersection tests */
CollisionResult intersect(const ICollider& a, const sf::Transform& wtA,
                          const ICollider& b, const sf::Transform& wtB);

} // namespace sfmx
