#pragma once

#include "core/platform/Prerequisites.h"
#include <SFML/Graphics/Transform.hpp>
#include <SFML/System/Vector2.hpp>

namespace sfmx {

// ── Shape type enum ──────────────────────────────────────────────────────

enum class ColliderType : uint8
{
  kCircle,
  kAABB,
  kOOBB,
  kPoint,
  kLine
};

// ── Collision result ─────────────────────────────────────────────────────

struct CollisionResult
{
  bool         hit         = false;
  sf::Vector2f normal;
  float        penetration = 0.f;
  explicit operator bool() const { return hit; }
};

// ── Math helpers ─────────────────────────────────────────────────────────

NODISCARD inline float
dot(sf::Vector2f a, sf::Vector2f b) { return a.x * b.x + a.y * b.y; }

NODISCARD inline float
lengthSquared(sf::Vector2f v) { return dot(v, v); }

NODISCARD inline float
length(sf::Vector2f v) { return std::sqrt(lengthSquared(v)); }

NODISCARD inline sf::Vector2f
normalize(sf::Vector2f v) {
  const float l = length(v);
  return (l > 0.f) ? v / l : sf::Vector2f{0.f, 0.f};
}

NODISCARD inline float
cross(sf::Vector2f a, sf::Vector2f b) { return a.x * b.y - a.y * b.x; }

// ── Collider hierarchy ───────────────────────────────────────────────────

class Collider
{
 public:
  virtual ~Collider() = default;
  NODISCARD virtual ColliderType getType() const = 0;
};

class CircleCollider : public Collider
{
 public:
  CircleCollider() = default;
  CircleCollider(sf::Vector2f center, float radius)
    : m_center(center), m_radius(radius) {}

  NODISCARD ColliderType getType() const override { return ColliderType::kCircle; }

  void setCenter(sf::Vector2f c) { m_center = c; }
  void setRadius(float r)        { m_radius = r; }
  NODISCARD sf::Vector2f getCenter() const { return m_center; }
  NODISCARD float getRadius()        const { return m_radius; }

 private:
  sf::Vector2f m_center = {0.f, 0.f};
  float        m_radius = 16.f;
};

class AABBCollider : public Collider
{
 public:
  AABBCollider() = default;
  AABBCollider(sf::Vector2f center, sf::Vector2f halfSize)
    : m_center(center), m_halfSize(halfSize) {}

  NODISCARD ColliderType getType() const override { return ColliderType::kAABB; }

  void setCenter(sf::Vector2f c)   { m_center = c; }
  void setHalfSize(sf::Vector2f h) { m_halfSize = h; }
  NODISCARD sf::Vector2f getCenter()   const { return m_center; }
  NODISCARD sf::Vector2f getHalfSize() const { return m_halfSize; }

 private:
  sf::Vector2f m_center   = {0.f, 0.f};
  sf::Vector2f m_halfSize = {16.f, 16.f};
};

class OOBBCollider : public Collider
{
 public:
  OOBBCollider() = default;
  OOBBCollider(sf::Vector2f center, sf::Vector2f halfSize)
    : m_center(center), m_halfSize(halfSize) {}

  NODISCARD ColliderType getType() const override { return ColliderType::kOOBB; }

  void setCenter(sf::Vector2f c)   { m_center = c; }
  void setHalfSize(sf::Vector2f h) { m_halfSize = h; }
  NODISCARD sf::Vector2f getCenter()   const { return m_center; }
  NODISCARD sf::Vector2f getHalfSize() const { return m_halfSize; }

 private:
  sf::Vector2f m_center   = {0.f, 0.f};
  sf::Vector2f m_halfSize = {16.f, 16.f};
};

class PointCollider : public Collider
{
 public:
  PointCollider() = default;
  explicit PointCollider(sf::Vector2f point) : m_point(point) {}

  NODISCARD ColliderType getType() const override { return ColliderType::kPoint; }

  void setPoint(sf::Vector2f p) { m_point = p; }
  NODISCARD sf::Vector2f getPoint() const { return m_point; }

 private:
  sf::Vector2f m_point = {0.f, 0.f};
};

class LineCollider : public Collider
{
 public:
  LineCollider() = default;
  LineCollider(sf::Vector2f start, sf::Vector2f end)
    : m_start(start), m_end(end) {}

  NODISCARD ColliderType getType() const override { return ColliderType::kLine; }

  void setStart(sf::Vector2f p) { m_start = p; }
  void setEnd(sf::Vector2f p)   { m_end = p; }
  NODISCARD sf::Vector2f getStart() const { return m_start; }
  NODISCARD sf::Vector2f getEnd()   const { return m_end; }

 private:
  sf::Vector2f m_start = {-8.f, 0.f};
  sf::Vector2f m_end   = { 8.f, 0.f};
};

// ── Single entry point for all collision tests ───────────────────────────

CollisionResult intersect(const Collider& a, const sf::Transform& wtA,
                          const Collider& b, const sf::Transform& wtB);

} // namespace sfmx
