#include "core/physics/Collider.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace sfmx
{
namespace
{

// -----------------------------------------------------------------------------
// Internal world-space shape representations
// -----------------------------------------------------------------------------

struct WorldCircle { sf::Vector2f center; float radius = 0.f; };
struct WorldAABB   { sf::Vector2f min, max; };

struct WorldOBB
{
  sf::Vector2f center, halfSize, axisX, axisY;
  void getCorners(sf::Vector2f (&out)[4]) const {
    const sf::Vector2f ex = axisX * halfSize.x;
    const sf::Vector2f ey = axisY * halfSize.y;
    out[0] = center - ex - ey;
    out[1] = center + ex - ey;
    out[2] = center + ex + ey;
    out[3] = center - ex + ey;
  }
};

struct WorldPoint { sf::Vector2f point; };
struct WorldLine  { sf::Vector2f start, end; };

// -----------------------------------------------------------------------------
// World-shape builders from Collider + Transform
// -----------------------------------------------------------------------------

/** @brief Builds a world-space circle from a CircleCollider + transform */
WorldCircle
buildWorldCircle(const CircleCollider& c, const sf::Transform& wt) {
  const sf::Vector2f s = {
    (wt.transformPoint({1.f, 0.f}) - wt.transformPoint({0.f, 0.f})).length(),
    (wt.transformPoint({0.f, 1.f}) - wt.transformPoint({0.f, 0.f})).length()
  };
  return {wt.transformPoint(c.getCenter()), c.getRadius() * (s.x + s.y) * 0.5f};
}

/** @brief Builds a world-space AABB from an AABBCollider + transform */
WorldAABB
buildWorldAABB(const AABBCollider& a, const sf::Transform& wt) {
  const auto hs = a.getHalfSize();
  const sf::Vector2f c[4] = {
    wt.transformPoint(a.getCenter() + sf::Vector2f{-hs.x, -hs.y}),
    wt.transformPoint(a.getCenter() + sf::Vector2f{ hs.x, -hs.y}),
    wt.transformPoint(a.getCenter() + sf::Vector2f{ hs.x,  hs.y}),
    wt.transformPoint(a.getCenter() + sf::Vector2f{-hs.x,  hs.y})
  };
  sf::Vector2f mn = c[0], mx = c[0];
  for (int i = 1; i < 4; ++i) {
    mn.x = std::min(mn.x, c[i].x); mn.y = std::min(mn.y, c[i].y);
    mx.x = std::max(mx.x, c[i].x); mx.y = std::max(mx.y, c[i].y);
  }
  return {mn, mx};
}

/** @brief Builds a world-space OBB from an OBBCollider + transform */
WorldOBB
buildWorldOBB(const OBBCollider& o, const sf::Transform& wt) {
  const sf::Vector2f ori = wt.transformPoint({0.f, 0.f});
  const sf::Vector2f ax  = (wt.transformPoint({1.f, 0.f}) - ori).normalized();
  const sf::Vector2f ay  = (wt.transformPoint({0.f, 1.f}) - ori).normalized();
  const float sx = (wt.transformPoint({1.f, 0.f}) - ori).length();
  const float sy = (wt.transformPoint({0.f, 1.f}) - ori).length();
  const auto hs = o.getHalfSize();
  return {wt.transformPoint(o.getCenter()), {hs.x * sx, hs.y * sy}, ax, ay};
}

/** @brief Builds a world-space point from a PointCollider + transform */
WorldPoint
buildWorldPoint(const PointCollider& p, const sf::Transform& wt) {
  return {wt.transformPoint(p.getPoint())};
}

/** @brief Builds a world-space line from a LineCollider + transform */
WorldLine
buildWorldLine(const LineCollider& l, const sf::Transform& wt) {
  return {wt.transformPoint(l.getStart()), wt.transformPoint(l.getEnd())};
}

// -----------------------------------------------------------------------------
// Math helpers
// -----------------------------------------------------------------------------

/** @brief Closest point on an AABB to a given point */
sf::Vector2f
closestPointOnAABB(const WorldAABB& aabb, sf::Vector2f p) {
  return {
    std::max(aabb.min.x, std::min(p.x, aabb.max.x)),
    std::max(aabb.min.y, std::min(p.y, aabb.max.y))
  };
}

/** @brief Closest point on a line segment to a given point */
sf::Vector2f
closestPointOnSegment(sf::Vector2f a, sf::Vector2f b, sf::Vector2f p) {
  const sf::Vector2f ab = b - a;
  const float abLenSq = ab.lengthSquared();
  if (abLenSq < 1e-8f) return a;
  const float t = std::clamp((p - a).dot(ab) / abLenSq, 0.f, 1.f);
  return a + ab * t;
}

/** @brief Project polygon corners onto an axis, returning min/max dot values */
void
projectCorners(const sf::Vector2f* corners, int count, sf::Vector2f axis,
               float& outMin, float& outMax) {
  outMin = corners[0].dot(axis);
  outMax = outMin;
  for (int i = 1; i < count; ++i) {
    const float d = corners[i].dot(axis);
    outMin = std::min(outMin, d);
    outMax = std::max(outMax, d);
  }
}

/** @brief Overlap amount of two intervals (negative → no overlap) */
float
axisOverlap(float minA, float maxA, float minB, float maxB) {
  return std::min(maxA, maxB) - std::max(minA, minB);
}

// -----------------------------------------------------------------------------
// SAT implementations
// -----------------------------------------------------------------------------

/** @brief Separating-axis test for OBB vs OBB */
bool
satOBB(const WorldOBB& a, const WorldOBB& b,
       sf::Vector2f& outNormal, float& outPenetration) {
  const std::array<sf::Vector2f, 4> axes = {a.axisX, a.axisY, b.axisX, b.axisY};
  sf::Vector2f aC[4], bC[4];
  a.getCorners(aC);
  b.getCorners(bC);
  outPenetration = std::numeric_limits<float>::max();
  for (const auto& axis : axes) {
    if (axis.lengthSquared() < 1e-6f) continue;
    const sf::Vector2f n = axis.normalized();
    float minA, maxA, minB, maxB;
    projectCorners(aC, 4, n, minA, maxA);
    projectCorners(bC, 4, n, minB, maxB);
    const float overlap = axisOverlap(minA, maxA, minB, maxB);
    if (overlap <= 0.f) return false;
    if (overlap < outPenetration) {
      outPenetration = overlap;
      const float cd = (a.center - b.center).dot(n);
      outNormal = (cd >= 0.f) ? -n : n;
    }
  }
  return true;
}

/** @brief Build an OBB from an AABB (identity axes) */
WorldOBB
worldOBBFromAABB(const WorldAABB& a) {
  return {(a.min + a.max) * 0.5f, (a.max - a.min) * 0.5f,
          {1.f, 0.f}, {0.f, 1.f}};
}

// -----------------------------------------------------------------------------
// Intersection functions (internal)
// -----------------------------------------------------------------------------

/** @brief Circle vs Circle */
CollisionResult
intersect(const WorldCircle& a, const WorldCircle& b) {
  const sf::Vector2f diff = b.center - a.center;
  const float dist = diff.length();
  const float radSum = a.radius + b.radius;
  CollisionResult res;
  if (dist < radSum && dist > 0.f) {
    res.hit = true; res.normal = diff / dist; res.penetration = radSum - dist;
  } else if (dist == 0.f) {
    res.hit = true; res.normal = {1.f, 0.f}; res.penetration = radSum;
  }
  return res;
}

/** @brief Circle vs AABB */
CollisionResult
intersect(const WorldCircle& a, const WorldAABB& b) {
  const sf::Vector2f closest = closestPointOnAABB(b, a.center);
  const sf::Vector2f diff = a.center - closest;
  const float dist = diff.length();
  CollisionResult res;
  if (dist < a.radius) {
    res.hit = true;
    if (dist > 0.f) {
      res.normal = diff / dist; res.penetration = a.radius - dist;
    } else {
      const float ox = std::min(a.center.x - b.min.x, b.max.x - a.center.x);
      const float oy = std::min(a.center.y - b.min.y, b.max.y - a.center.y);
      if (ox < oy) {
        res.normal = {a.center.x < (b.min.x + b.max.x) * 0.5f ? -1.f : 1.f, 0.f};
        res.penetration = ox + a.radius;
      } else {
        res.normal = {0.f, a.center.y < (b.min.y + b.max.y) * 0.5f ? -1.f : 1.f};
        res.penetration = oy + a.radius;
      }
    }
  }
  return res;
}

/** @brief Circle vs OBB (SAT in local space of the OBB) */
CollisionResult
intersect(const WorldCircle& a, const WorldOBB& b) {
  const sf::Vector2f lc = {(a.center - b.center).dot(b.axisX),
                           (a.center - b.center).dot(b.axisY)};
  const WorldAABB la = {{-b.halfSize.x, -b.halfSize.y}, {b.halfSize.x, b.halfSize.y}};
  const sf::Vector2f closest = closestPointOnAABB(la, lc);
  const sf::Vector2f diff = lc - closest;
  const float dist = diff.length();
  CollisionResult res;
  if (dist < a.radius) {
    res.hit = true;
    sf::Vector2f ln;
    if (dist > 0.f) {
      ln = diff / dist; res.penetration = a.radius - dist;
    } else {
      const float ox = std::min(lc.x + b.halfSize.x, b.halfSize.x - lc.x);
      const float oy = std::min(lc.y + b.halfSize.y, b.halfSize.y - lc.y);
      if (ox < oy) {
        ln = {lc.x < 0.f ? -1.f : 1.f, 0.f}; res.penetration = ox + a.radius;
      } else {
        ln = {0.f, lc.y < 0.f ? -1.f : 1.f}; res.penetration = oy + a.radius;
      }
    }
    res.normal = (b.axisX * ln.x + b.axisY * ln.y).normalized();
    if (res.normal.lengthSquared() == 0.f) res.normal = {1.f, 0.f};
  }
  return res;
}

/** @brief Circle vs Point */
CollisionResult
intersect(const WorldCircle& a, const WorldPoint& b) {
  const sf::Vector2f diff = b.point - a.center;
  const float dist = diff.length();
  CollisionResult res;
  if (dist < a.radius) {
    res.hit = true;
    if (dist > 0.f) { res.normal = diff / dist; res.penetration = a.radius - dist; }
    else { res.normal = {1.f, 0.f}; res.penetration = a.radius; }
  }
  return res;
}

/** @brief Circle vs Line */
CollisionResult
intersect(const WorldCircle& a, const WorldLine& b) {
  const sf::Vector2f closest = closestPointOnSegment(b.start, b.end, a.center);
  const sf::Vector2f diff = a.center - closest;
  const float dist = diff.length();
  CollisionResult res;
  if (dist < a.radius) {
    res.hit = true;
    if (dist > 0.f) { res.normal = diff.normalized(); res.penetration = a.radius - dist; }
    else { res.normal = (b.end - b.start).normalized(); if (res.normal.lengthSquared() == 0.f) res.normal = {1.f, 0.f}; res.penetration = a.radius; }
  }
  return res;
}

/** @brief AABB vs AABB */
CollisionResult
intersect(const WorldAABB& a, const WorldAABB& b) {
  const float ox = axisOverlap(a.min.x, a.max.x, b.min.x, b.max.x);
  const float oy = axisOverlap(a.min.y, a.max.y, b.min.y, b.max.y);
  CollisionResult res;
  if (ox > 0.f && oy > 0.f) {
    res.hit = true;
    const float ca = (a.min.x + a.max.x) * 0.5f, cb = (b.min.x + b.max.x) * 0.5f;
    if (ox < oy) { res.normal = {ca < cb ? 1.f : -1.f, 0.f}; res.penetration = ox; }
    else        { res.normal = {0.f, (a.min.y + a.max.y) * 0.5f < (b.min.y + b.max.y) * 0.5f ? 1.f : -1.f}; res.penetration = oy; }
  }
  return res;
}

/** @brief AABB vs OBB (delegates to SAT by converting AABB to OBB first) */
CollisionResult
intersect(const WorldAABB& a, const WorldOBB& b) {
  sf::Vector2f n; float p;
  if (satOBB(worldOBBFromAABB(a), b, n, p)) return {true, n, p};
  return {};
}

/** @brief AABB vs Point */
CollisionResult
intersect(const WorldAABB& a, const WorldPoint& b) {
  CollisionResult res;
  if (b.point.x >= a.min.x && b.point.x <= a.max.x &&
      b.point.y >= a.min.y && b.point.y <= a.max.y) {
    res.hit = true;
    const float tl = b.point.x - a.min.x, tr = a.max.x - b.point.x;
    const float tt = b.point.y - a.min.y, tb = a.max.y - b.point.y;
    const float mx = std::min(tl, tr), my = std::min(tt, tb);
    if (mx < my) { res.normal = tl < tr ? sf::Vector2f{-1.f, 0.f} : sf::Vector2f{1.f, 0.f}; res.penetration = mx; }
    else         { res.normal = tt < tb ? sf::Vector2f{0.f, -1.f} : sf::Vector2f{0.f, 1.f}; res.penetration = my; }
  }
  return res;
}

/** @brief AABB vs Line */
CollisionResult
intersect(const WorldAABB& a, const WorldLine& b) {
  auto inside = [&](sf::Vector2f p) {
    return p.x >= a.min.x && p.x <= a.max.x && p.y >= a.min.y && p.y <= a.max.y;
  };
  if (inside(b.start) || inside(b.end)) {
    CollisionResult res;
    res.hit = true;
    const sf::Vector2f mid = (b.start + b.end) * 0.5f;
    const float tl = mid.x - a.min.x, tr = a.max.x - mid.x;
    const float tt = mid.y - a.min.y, tb = a.max.y - mid.y;
    const float mx = std::min(tl, tr), my = std::min(tt, tb);
    if (mx < my) { res.normal = tl < tr ? sf::Vector2f{-1.f, 0.f} : sf::Vector2f{1.f, 0.f}; res.penetration = mx; }
    else         { res.normal = tt < tb ? sf::Vector2f{0.f, -1.f} : sf::Vector2f{0.f, 1.f}; res.penetration = my; }
    return res;
  }
  auto segX = [](sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, sf::Vector2f p4) {
    return ((p4 - p3).cross(p1 - p3) * (p4 - p3).cross(p2 - p3) < 0.f) &&
           ((p2 - p1).cross(p3 - p1) * (p2 - p1).cross(p4 - p1) < 0.f);
  };
  const std::array<sf::Vector2f, 4> cr = {{
    {a.min.x, a.min.y}, {a.max.x, a.min.y}, {a.max.x, a.max.y}, {a.min.x, a.max.y}
  }};
  for (size_t i = 0; i < 4; ++i) {
    const auto& p1 = cr[i], p2 = cr[(i + 1) % 4];
    if (segX(b.start, b.end, p1, p2)) {
      CollisionResult res;
      res.hit = true;
      const sf::Vector2f ed = p2 - p1;
      res.normal = sf::Vector2f{-ed.y, ed.x}.normalized();
      res.penetration = 1.f;
      return res;
    }
  }
  return {};
}

/** @brief OBB vs OBB (delegates to SAT) */
CollisionResult
intersect(const WorldOBB& a, const WorldOBB& b) {
  sf::Vector2f n; float p;
  if (satOBB(a, b, n, p)) return {true, n, p};
  return {};
}

/** @brief OBB vs Point (transform into OBB local space, test as AABB) */
CollisionResult
intersect(const WorldOBB& a, const WorldPoint& b) {
  const sf::Vector2f lp = {(b.point - a.center).dot(a.axisX), (b.point - a.center).dot(a.axisY)};
  return intersect(WorldAABB{{-a.halfSize.x, -a.halfSize.y}, {a.halfSize.x, a.halfSize.y}}, WorldPoint{lp});
}

/** @brief OBB vs Line (transform into OBB local space, test as AABB-hit) */
CollisionResult
intersect(const WorldOBB& a, const WorldLine& b) {
  const sf::Vector2f ls = {(b.start - a.center).dot(a.axisX), (b.start - a.center).dot(a.axisY)};
  const sf::Vector2f le = {(b.end   - a.center).dot(a.axisX), (b.end   - a.center).dot(a.axisY)};
  return intersect(WorldAABB{{-a.halfSize.x, -a.halfSize.y}, {a.halfSize.x, a.halfSize.y}}, WorldLine{ls, le});
}

/** @brief Point vs Point (exact-position match) */
CollisionResult
intersect(const WorldPoint& a, const WorldPoint& b) {
  CollisionResult res;
  if ((b.point - a.point).lengthSquared() < 1e-6f) {
    res.hit = true; res.normal = {1.f, 0.f}; res.penetration = 0.f;
  }
  return res;
}

/** @brief Point vs Line (point lies exactly on segment) */
CollisionResult
intersect(const WorldPoint& a, const WorldLine& b) {
  const sf::Vector2f ab = b.end - b.start;
  const float abls = ab.lengthSquared();
  if (abls < 1e-6f) return {};
  const float t = std::clamp((a.point - b.start).dot(ab) / abls, 0.f, 1.f);
  if ((a.point - (b.start + ab * t)).lengthSquared() < 1e-6f) {
    CollisionResult res;
    res.hit = true; res.normal = sf::Vector2f{-ab.y, ab.x}.normalized(); res.penetration = 0.f;
    return res;
  }
  return {};
}

/** @brief Line vs Line (segment intersection) */
CollisionResult
intersect(const WorldLine& a, const WorldLine& b) {
  const sf::Vector2f d1 = a.end - a.start, d2 = b.end - b.start;
  const float denom = d1.cross(d2);
  if (std::abs(denom) < 1e-8f) return {};
  const float t = (b.start - a.start).cross(d2) / denom;
  const float u = (b.start - a.start).cross(d1) / denom;
  if (t >= 0.f && t <= 1.f && u >= 0.f && u <= 1.f) {
    CollisionResult res;
    res.hit = true; res.normal = sf::Vector2f{-d2.y, d2.x}.normalized(); res.penetration = 0.f;
    return res;
  }
  return {};
}

} // namespace

// -----------------------------------------------------------------------------
// Public dispatch
// -----------------------------------------------------------------------------

/** @brief Public entry point: any Collider pair → intersection result */
CollisionResult
intersect(const ICollider& a, const sf::Transform& wtA,
          const ICollider& b, const sf::Transform& wtB) {

  UUID aUUID = a.getTypeId();
  UUID bUUID = b.getTypeId();
  if (TypeTraits<CircleCollider>::getTypeId() == aUUID) {
    const auto& ca = static_cast<const CircleCollider&>(a);
    const WorldCircle wa = buildWorldCircle(ca, wtA);
    if (TypeTraits<CircleCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldCircle(static_cast<const CircleCollider&>(b), wtB));
    }
    else if (TypeTraits<AABBCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldAABB(static_cast<const AABBCollider&>(b), wtB));
    }
    else if (TypeTraits<OBBCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldOBB(static_cast<const OBBCollider&>(b), wtB));
    }
    else if (TypeTraits<PointCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldPoint(static_cast<const PointCollider&>(b), wtB));
    }
    else if (TypeTraits<LineCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldLine(static_cast<const LineCollider&>(b), wtB));
    }      
  }
  else if (TypeTraits<AABBCollider>::getTypeId() == aUUID) {
    const auto& ca = static_cast<const AABBCollider&>(a);
    const WorldAABB wa = buildWorldAABB(ca, wtA);
    if (TypeTraits<CircleCollider>::getTypeId() == bUUID) {
      return intersect(buildWorldCircle(static_cast<const CircleCollider&>(b), wtB), wa);
    }
    else if (TypeTraits<AABBCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldAABB(static_cast<const AABBCollider&>(b), wtB));
    }
    else if (TypeTraits<OBBCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldOBB(static_cast<const OBBCollider&>(b), wtB));
    }
    else if (TypeTraits<PointCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldPoint(static_cast<const PointCollider&>(b), wtB));
    }
    else if (TypeTraits<LineCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldLine(static_cast<const LineCollider&>(b), wtB));
    } 
  }
  else if (TypeTraits<OBBCollider>::getTypeId() == aUUID) {
    const auto& ca = static_cast<const OBBCollider&>(a);
    const WorldOBB wa = buildWorldOBB(ca, wtA);
    if (TypeTraits<CircleCollider>::getTypeId() == bUUID) {
      return intersect(buildWorldCircle(static_cast<const CircleCollider&>(b), wtB), wa);
    }
    else if (TypeTraits<AABBCollider>::getTypeId() == bUUID) {
      return intersect(buildWorldAABB(static_cast<const AABBCollider&>(b), wtB), wa);
    }
    else if (TypeTraits<OBBCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldOBB(static_cast<const OBBCollider&>(b), wtB));
    }
    else if (TypeTraits<PointCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldPoint(static_cast<const PointCollider&>(b), wtB));
    }
    else if (TypeTraits<LineCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldLine(static_cast<const LineCollider&>(b), wtB));
    } 
  }
  else if (TypeTraits<PointCollider>::getTypeId() == aUUID) {
    const auto& ca = static_cast<const PointCollider&>(a);
    const WorldPoint wa = buildWorldPoint(ca, wtA);
    if (TypeTraits<CircleCollider>::getTypeId() == bUUID) {
      return intersect(buildWorldCircle(static_cast<const CircleCollider&>(b), wtB), wa);
    }
    else if (TypeTraits<AABBCollider>::getTypeId() == bUUID) {
      return intersect(buildWorldAABB(static_cast<const AABBCollider&>(b), wtB), wa);
    }
    else if (TypeTraits<OBBCollider>::getTypeId() == bUUID) {
      return intersect(buildWorldOBB(static_cast<const OBBCollider&>(b), wtB), wa);
    }
    else if (TypeTraits<PointCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldPoint(static_cast<const PointCollider&>(b), wtB));
    }
    else if (TypeTraits<LineCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldLine(static_cast<const LineCollider&>(b), wtB));
    } 
  }
  else if (TypeTraits<LineCollider>::getTypeId() == aUUID) {
    const auto& ca = static_cast<const LineCollider&>(a);
    const WorldLine wa = buildWorldLine(ca, wtA);
    if (TypeTraits<CircleCollider>::getTypeId() == bUUID) {
      return intersect(buildWorldCircle(static_cast<const CircleCollider&>(b), wtB), wa);
    }
    else if (TypeTraits<AABBCollider>::getTypeId() == bUUID) {
      return intersect(buildWorldAABB(static_cast<const AABBCollider&>(b), wtB), wa);
    }
    else if (TypeTraits<OBBCollider>::getTypeId() == bUUID) {
      return intersect(buildWorldOBB(static_cast<const OBBCollider&>(b), wtB), wa);
    }
    else if (TypeTraits<PointCollider>::getTypeId() == bUUID) {
      return intersect(buildWorldPoint(static_cast<const PointCollider&>(b), wtB), wa);
    }
    else if (TypeTraits<LineCollider>::getTypeId() == bUUID) {
      return intersect(wa, buildWorldLine(static_cast<const LineCollider&>(b), wtB));
    } 
  }

  return {};
}

} // namespace sfmx
