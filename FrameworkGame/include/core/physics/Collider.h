/************************************************************************/
/**
 * @file Collider.h
 * @author Swampertor
 * @date 2026/06/16
 * @brief  Collider shape hierarchy and unified intersection dispatch.
 */
/************************************************************************/
#pragma once

#include "core/platform/Prerequisites.h"
#include <SFML/Graphics/Transform.hpp>
#include <SFML/System/Vector2.hpp>

namespace sfmx
{

// Math helpers

/** @brief Dot product of two 2D vectors */
NODISCARD inline float
dot(sf::Vector2f a, sf::Vector2f b) { return a.x * b.x + a.y * b.y; }

/** @brief Squared length (avoids sqrt) */
NODISCARD inline float
lengthSquared(sf::Vector2f v) { return dot(v, v); }

/** @brief Euclidean length */
NODISCARD inline float
length(sf::Vector2f v) { return std::sqrt(lengthSquared(v)); }

/** @brief Unit-length vector (zero-safe) */
NODISCARD inline sf::Vector2f
normalize(sf::Vector2f v) {
  const float l = length(v);
  return (l > 0.f) ? v / l : sf::Vector2f{0.f, 0.f};
}

/** @brief 2D cross product (scalar) */
NODISCARD inline float
cross(sf::Vector2f a, sf::Vector2f b) { return a.x * b.y - a.y * b.x; }

// Shape type enum

/** @brief Discriminator for the concrete collider type under Collider* */
enum class ColliderType : uint8
{
  kCircle,
  kAABB,
  kOOBB,
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
  explicit operator bool() const { return hit; }
};

// Collider hierarchy

/**
 * @brief Abstract base for all collider shapes.
 *
 * Subclasses carry local-space data.  The free function intersect()
 * accepts any two Collider subtypes together with their world transforms.
 */
class Collider
{
 public:
  virtual ~Collider() = default;
  /** @brief Returns the concrete shape type for dispatch */
  NODISCARD virtual ColliderType getType() const = 0;
};

/** @brief Circle defined by a center point and radius */
class CircleCollider : public Collider
{
 public:
  CircleCollider() = default;
  /** @brief Construct with local-space center and radius */
  CircleCollider(sf::Vector2f center, float radius)
    : m_center(center), m_radius(radius) {}

  NODISCARD ColliderType getType() const override { return ColliderType::kCircle; }

  /** @brief Set the local-space center position */
  void setCenter(sf::Vector2f c) { m_center = c; }
  /** @brief Set the circle radius */
  void setRadius(float r)        { m_radius = r; }
  /** @brief Local-space center position */
  NODISCARD sf::Vector2f getCenter() const { return m_center; }
  /** @brief The circle radius */
  NODISCARD float getRadius()        const { return m_radius; }

 private:
  sf::Vector2f m_center = {0.f, 0.f};
  float        m_radius = 16.f;
};

/** @brief Axis-aligned bounding box with center + half-size */
class AABBCollider : public Collider
{
 public:
  AABBCollider() = default;
  /** @brief Construct with local-space center and half-extents */
  AABBCollider(sf::Vector2f center, sf::Vector2f halfSize)
    : m_center(center), m_halfSize(halfSize) {}

  NODISCARD ColliderType getType() const override { return ColliderType::kAABB; }

  /** @brief Set the local-space center position */
  void setCenter(sf::Vector2f c)   { m_center = c; }
  /** @brief Set the half-size extents */
  void setHalfSize(sf::Vector2f h) { m_halfSize = h; }
  /** @brief Local-space center position */
  NODISCARD sf::Vector2f getCenter()   const { return m_center; }
  /** @brief Half-size extents */
  NODISCARD sf::Vector2f getHalfSize() const { return m_halfSize; }

 private:
  sf::Vector2f m_center   = {0.f, 0.f};
  sf::Vector2f m_halfSize = {16.f, 16.f};
};

/** @brief Oriented bounding box (AABB rotated via its world transform) */
class OOBBCollider : public Collider
{
 public:
  OOBBCollider() = default;
  /** @brief Construct with local-space center and half-extents */
  OOBBCollider(sf::Vector2f center, sf::Vector2f halfSize)
    : m_center(center), m_halfSize(halfSize) {}

  NODISCARD ColliderType getType() const override { return ColliderType::kOOBB; }

  /** @brief Set the local-space center position */
  void setCenter(sf::Vector2f c)   { m_center = c; }
  /** @brief Set the half-size extents */
  void setHalfSize(sf::Vector2f h) { m_halfSize = h; }
  /** @brief Local-space center position */
  NODISCARD sf::Vector2f getCenter()   const { return m_center; }
  /** @brief Half-size extents */
  NODISCARD sf::Vector2f getHalfSize() const { return m_halfSize; }

 private:
  sf::Vector2f m_center   = {0.f, 0.f};
  sf::Vector2f m_halfSize = {16.f, 16.f};
};

/** @brief Zero-area point collider */
class PointCollider : public Collider
{
 public:
  PointCollider() = default;
  /** @brief Construct at a given local-space point */
  explicit PointCollider(sf::Vector2f point) : m_point(point) {}

  NODISCARD ColliderType getType() const override { return ColliderType::kPoint; }

  /** @brief Set the local-space point position */
  void setPoint(sf::Vector2f p) { m_point = p; }
  /** @brief The local-space point position */
  NODISCARD sf::Vector2f getPoint() const { return m_point; }

 private:
  sf::Vector2f m_point = {0.f, 0.f};
};

/** @brief Line segment collider (start / end) */
class LineCollider : public Collider
{
 public:
  LineCollider() = default;
  /** @brief Construct with local-space start and end points */
  LineCollider(sf::Vector2f start, sf::Vector2f end)
    : m_start(start), m_end(end) {}

  NODISCARD ColliderType getType() const override { return ColliderType::kLine; }

  /** @brief Set the line segment start point */
  void setStart(sf::Vector2f p) { m_start = p; }
  /** @brief Set the line segment end point */
  void setEnd(sf::Vector2f p)   { m_end = p; }
  /** @brief The line segment start point */
  NODISCARD sf::Vector2f getStart() const { return m_start; }
  /** @brief The line segment end point */
  NODISCARD sf::Vector2f getEnd()   const { return m_end; }

 private:
  sf::Vector2f m_start = {-8.f, 0.f};
  sf::Vector2f m_end   = { 8.f, 0.f};
};

// Public dispatch

/** @brief Single entry point for all collider pair intersection tests */
CollisionResult intersect(const Collider& a, const sf::Transform& wtA,
                          const Collider& b, const sf::Transform& wtB);

} // namespace sfmx
