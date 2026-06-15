#include "core/physics/Collider.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace sfmx {
namespace {

// ===========================================================================
// Internal world-space shape representations
// ===========================================================================

struct WorldCircle { sf::Vector2f center; float radius = 0.f; };
struct WorldAABB   { sf::Vector2f min, max; };

struct WorldOOBB
{
  sf::Vector2f center, halfSize, axisX, axisY;
  void getCorners(sf::Vector2f* out) const {
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

// ===========================================================================
// World-shape builders from Collider + Transform
// ===========================================================================

WorldCircle buildWorldCircle(const CircleCollider& c, const sf::Transform& wt) {
  const sf::Vector2f s = {
    length(wt.transformPoint({1.f, 0.f}) - wt.transformPoint({0.f, 0.f})),
    length(wt.transformPoint({0.f, 1.f}) - wt.transformPoint({0.f, 0.f}))
  };
  return {wt.transformPoint(c.getCenter()), c.getRadius() * (s.x + s.y) * 0.5f};
}

WorldAABB buildWorldAABB(const AABBCollider& a, const sf::Transform& wt) {
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

WorldOOBB buildWorldOOBB(const OOBBCollider& o, const sf::Transform& wt) {
  const sf::Vector2f ori = wt.transformPoint({0.f, 0.f});
  const sf::Vector2f ax  = normalize(wt.transformPoint({1.f, 0.f}) - ori);
  const sf::Vector2f ay  = normalize(wt.transformPoint({0.f, 1.f}) - ori);
  const float sx = length(wt.transformPoint({1.f, 0.f}) - ori);
  const float sy = length(wt.transformPoint({0.f, 1.f}) - ori);
  const auto hs = o.getHalfSize();
  return {wt.transformPoint(o.getCenter()), {hs.x * sx, hs.y * sy}, ax, ay};
}

WorldPoint buildWorldPoint(const PointCollider& p, const sf::Transform& wt) {
  return {wt.transformPoint(p.getPoint())};
}

WorldLine buildWorldLine(const LineCollider& l, const sf::Transform& wt) {
  return {wt.transformPoint(l.getStart()), wt.transformPoint(l.getEnd())};
}

// ===========================================================================
// Math helpers
// ===========================================================================

sf::Vector2f
closestPointOnAABB(const WorldAABB& aabb, sf::Vector2f p) {
  return {
    std::max(aabb.min.x, std::min(p.x, aabb.max.x)),
    std::max(aabb.min.y, std::min(p.y, aabb.max.y))
  };
}

sf::Vector2f
closestPointOnSegment(sf::Vector2f a, sf::Vector2f b, sf::Vector2f p) {
  const sf::Vector2f ab = b - a;
  const float abLenSq = lengthSquared(ab);
  if (abLenSq < 1e-8f) return a;
  const float t = std::clamp(dot(p - a, ab) / abLenSq, 0.f, 1.f);
  return a + ab * t;
}

void
projectCorners(const sf::Vector2f* corners, int count, sf::Vector2f axis,
               float& outMin, float& outMax) {
  outMin = dot(corners[0], axis);
  outMax = outMin;
  for (int i = 1; i < count; ++i) {
    const float d = dot(corners[i], axis);
    outMin = std::min(outMin, d);
    outMax = std::max(outMax, d);
  }
}

float
axisOverlap(float minA, float maxA, float minB, float maxB) {
  return std::min(maxA, maxB) - std::max(minA, minB);
}

// SAT for OOBB vs OOBB
bool
satOOBB(const WorldOOBB& a, const WorldOOBB& b,
        sf::Vector2f& outNormal, float& outPenetration) {
  const std::array<sf::Vector2f, 4> axes = {a.axisX, a.axisY, b.axisX, b.axisY};
  sf::Vector2f aC[4], bC[4];
  a.getCorners(aC);
  b.getCorners(bC);
  outPenetration = std::numeric_limits<float>::max();
  for (const auto& axis : axes) {
    if (lengthSquared(axis) < 1e-6f) continue;
    const sf::Vector2f n = normalize(axis);
    float minA, maxA, minB, maxB;
    projectCorners(aC, 4, n, minA, maxA);
    projectCorners(bC, 4, n, minB, maxB);
    const float overlap = axisOverlap(minA, maxA, minB, maxB);
    if (overlap <= 0.f) return false;
    if (overlap < outPenetration) {
      outPenetration = overlap;
      const float cd = dot(a.center - b.center, n);
      outNormal = (cd >= 0.f) ? -n : n;
    }
  }
  return true;
}

// SAT for AABB vs OOBB
bool
satAABBOOBB(const WorldAABB& a, const WorldOOBB& b,
            sf::Vector2f& outNormal, float& outPenetration) {
  const std::array<sf::Vector2f, 4> axes = {
    sf::Vector2f{1.f, 0.f}, sf::Vector2f{0.f, 1.f}, b.axisX, b.axisY
  };
  const std::array<sf::Vector2f, 4> aC = {{
    {a.min.x, a.min.y}, {a.max.x, a.min.y},
    {a.max.x, a.max.y}, {a.min.x, a.max.y}
  }};
  sf::Vector2f bC[4];
  b.getCorners(bC);
  outPenetration = std::numeric_limits<float>::max();
  for (const auto& axis : axes) {
    if (lengthSquared(axis) < 1e-6f) continue;
    const sf::Vector2f n = normalize(axis);
    float minA, maxA, minB, maxB;
    projectCorners(aC.data(), 4, n, minA, maxA);
    projectCorners(bC, 4, n, minB, maxB);
    const float overlap = axisOverlap(minA, maxA, minB, maxB);
    if (overlap <= 0.f) return false;
    if (overlap < outPenetration) {
      outPenetration = overlap;
      const float cd = dot((a.min + a.max) * 0.5f - b.center, n);
      outNormal = (cd >= 0.f) ? -n : n;
    }
  }
  return true;
}

// ===========================================================================
// Intersection functions (internal)
// ===========================================================================

CollisionResult intersect(const WorldCircle& a, const WorldCircle& b) {
  const sf::Vector2f diff = b.center - a.center;
  const float dist = length(diff);
  const float radSum = a.radius + b.radius;
  CollisionResult res;
  if (dist < radSum && dist > 0.f) {
    res.hit = true; res.normal = diff / dist; res.penetration = radSum - dist;
  } else if (dist == 0.f) {
    res.hit = true; res.normal = {1.f, 0.f}; res.penetration = radSum;
  }
  return res;
}

CollisionResult intersect(const WorldCircle& a, const WorldAABB& b) {
  const sf::Vector2f closest = closestPointOnAABB(b, a.center);
  const sf::Vector2f diff = a.center - closest;
  const float dist = length(diff);
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

CollisionResult intersect(const WorldCircle& a, const WorldOOBB& b) {
  const sf::Vector2f lc = {dot(a.center - b.center, b.axisX),
                           dot(a.center - b.center, b.axisY)};
  const WorldAABB la = {{-b.halfSize.x, -b.halfSize.y}, {b.halfSize.x, b.halfSize.y}};
  const sf::Vector2f closest = closestPointOnAABB(la, lc);
  const sf::Vector2f diff = lc - closest;
  const float dist = length(diff);
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
    res.normal = normalize(b.axisX * ln.x + b.axisY * ln.y);
    if (lengthSquared(res.normal) == 0.f) res.normal = {1.f, 0.f};
  }
  return res;
}

CollisionResult intersect(const WorldCircle& a, const WorldPoint& b) {
  const sf::Vector2f diff = b.point - a.center;
  const float dist = length(diff);
  CollisionResult res;
  if (dist < a.radius) {
    res.hit = true;
    if (dist > 0.f) { res.normal = diff / dist; res.penetration = a.radius - dist; }
    else { res.normal = {1.f, 0.f}; res.penetration = a.radius; }
  }
  return res;
}

CollisionResult intersect(const WorldCircle& a, const WorldLine& b) {
  const sf::Vector2f closest = closestPointOnSegment(b.start, b.end, a.center);
  const sf::Vector2f diff = a.center - closest;
  const float dist = length(diff);
  CollisionResult res;
  if (dist < a.radius) {
    res.hit = true;
    if (dist > 0.f) { res.normal = normalize(diff); res.penetration = a.radius - dist; }
    else { res.normal = normalize(b.end - b.start); if (lengthSquared(res.normal) == 0.f) res.normal = {1.f, 0.f}; res.penetration = a.radius; }
  }
  return res;
}

CollisionResult intersect(const WorldAABB& a, const WorldAABB& b) {
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

CollisionResult intersect(const WorldAABB& a, const WorldOOBB& b) {
  sf::Vector2f n; float p;
  if (satAABBOOBB(a, b, n, p)) return {true, n, p};
  return {};
}

CollisionResult intersect(const WorldAABB& a, const WorldPoint& b) {
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

CollisionResult intersect(const WorldAABB& a, const WorldLine& b) {
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
    return (cross(p4 - p3, p1 - p3) * cross(p4 - p3, p2 - p3) < 0.f) &&
           (cross(p2 - p1, p3 - p1) * cross(p2 - p1, p4 - p1) < 0.f);
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
      res.normal = normalize(sf::Vector2f{-ed.y, ed.x});
      res.penetration = 1.f;
      return res;
    }
  }
  return {};
}

CollisionResult intersect(const WorldOOBB& a, const WorldOOBB& b) {
  sf::Vector2f n; float p;
  if (satOOBB(a, b, n, p)) return {true, n, p};
  return {};
}

CollisionResult intersect(const WorldOOBB& a, const WorldPoint& b) {
  const sf::Vector2f lp = {dot(b.point - a.center, a.axisX), dot(b.point - a.center, a.axisY)};
  return intersect(WorldAABB{{-a.halfSize.x, -a.halfSize.y}, {a.halfSize.x, a.halfSize.y}}, WorldPoint{lp});
}

CollisionResult intersect(const WorldOOBB& a, const WorldLine& b) {
  const sf::Vector2f ls = {dot(b.start - a.center, a.axisX), dot(b.start - a.center, a.axisY)};
  const sf::Vector2f le = {dot(b.end   - a.center, a.axisX), dot(b.end   - a.center, a.axisY)};
  return intersect(WorldAABB{{-a.halfSize.x, -a.halfSize.y}, {a.halfSize.x, a.halfSize.y}}, WorldLine{ls, le});
}

CollisionResult intersect(const WorldPoint& a, const WorldPoint& b) {
  CollisionResult res;
  if (lengthSquared(b.point - a.point) < 1e-6f) {
    res.hit = true; res.normal = {1.f, 0.f}; res.penetration = 0.f;
  }
  return res;
}

CollisionResult intersect(const WorldPoint& a, const WorldLine& b) {
  const sf::Vector2f ab = b.end - b.start;
  const float abls = lengthSquared(ab);
  if (abls < 1e-6f) return {};
  const float t = std::clamp(dot(a.point - b.start, ab) / abls, 0.f, 1.f);
  if (lengthSquared(a.point - (b.start + ab * t)) < 1e-6f) {
    CollisionResult res;
    res.hit = true; res.normal = normalize(sf::Vector2f{-ab.y, ab.x}); res.penetration = 0.f;
    return res;
  }
  return {};
}

CollisionResult intersect(const WorldLine& a, const WorldLine& b) {
  const sf::Vector2f d1 = a.end - a.start, d2 = b.end - b.start;
  const float denom = cross(d1, d2);
  if (std::abs(denom) < 1e-8f) return {};
  const float t = cross(b.start - a.start, d2) / denom;
  const float u = cross(b.start - a.start, d1) / denom;
  if (t >= 0.f && t <= 1.f && u >= 0.f && u <= 1.f) {
    CollisionResult res;
    res.hit = true; res.normal = normalize(sf::Vector2f{-d2.y, d2.x}); res.penetration = 0.f;
    return res;
  }
  return {};
}

} // namespace

// ===========================================================================
// Public dispatch: any Collider pair → intersection result
// ===========================================================================

CollisionResult
intersect(const Collider& a, const sf::Transform& wtA,
          const Collider& b, const sf::Transform& wtB) {
  // Build world shapes and dispatch
  switch (a.getType()) {
    case ColliderType::kCircle: {
      const auto& ca = static_cast<const CircleCollider&>(a);
      const WorldCircle wa = buildWorldCircle(ca, wtA);
      switch (b.getType()) {
        case ColliderType::kCircle: return intersect(wa, buildWorldCircle(static_cast<const CircleCollider&>(b), wtB));
        case ColliderType::kAABB:   return intersect(wa, buildWorldAABB(static_cast<const AABBCollider&>(b), wtB));
        case ColliderType::kOOBB:   return intersect(wa, buildWorldOOBB(static_cast<const OOBBCollider&>(b), wtB));
        case ColliderType::kPoint:  return intersect(wa, buildWorldPoint(static_cast<const PointCollider&>(b), wtB));
        case ColliderType::kLine:   return intersect(wa, buildWorldLine(static_cast<const LineCollider&>(b), wtB));
      }
      break;
    }
    case ColliderType::kAABB: {
      const auto& ca = static_cast<const AABBCollider&>(a);
      const WorldAABB wa = buildWorldAABB(ca, wtA);
      switch (b.getType()) {
        case ColliderType::kCircle:
          return intersect(buildWorldCircle(static_cast<const CircleCollider&>(b), wtB), wa);
        case ColliderType::kAABB:   return intersect(wa, buildWorldAABB(static_cast<const AABBCollider&>(b), wtB));
        case ColliderType::kOOBB:   return intersect(wa, buildWorldOOBB(static_cast<const OOBBCollider&>(b), wtB));
        case ColliderType::kPoint:  return intersect(wa, buildWorldPoint(static_cast<const PointCollider&>(b), wtB));
        case ColliderType::kLine:   return intersect(wa, buildWorldLine(static_cast<const LineCollider&>(b), wtB));
      }
      break;
    }
    case ColliderType::kOOBB: {
      const auto& ca = static_cast<const OOBBCollider&>(a);
      const WorldOOBB wa = buildWorldOOBB(ca, wtA);
      switch (b.getType()) {
        case ColliderType::kCircle:
          return intersect(buildWorldCircle(static_cast<const CircleCollider&>(b), wtB), wa);
        case ColliderType::kAABB:
          return intersect(buildWorldAABB(static_cast<const AABBCollider&>(b), wtB), wa);
        case ColliderType::kOOBB:   return intersect(wa, buildWorldOOBB(static_cast<const OOBBCollider&>(b), wtB));
        case ColliderType::kPoint:  return intersect(wa, buildWorldPoint(static_cast<const PointCollider&>(b), wtB));
        case ColliderType::kLine:   return intersect(wa, buildWorldLine(static_cast<const LineCollider&>(b), wtB));
      }
      break;
    }
    case ColliderType::kPoint: {
      const auto& ca = static_cast<const PointCollider&>(a);
      const WorldPoint wa = buildWorldPoint(ca, wtA);
      switch (b.getType()) {
        case ColliderType::kCircle:
          return intersect(buildWorldCircle(static_cast<const CircleCollider&>(b), wtB), wa);
        case ColliderType::kAABB:
          return intersect(buildWorldAABB(static_cast<const AABBCollider&>(b), wtB), wa);
        case ColliderType::kOOBB:
          return intersect(buildWorldOOBB(static_cast<const OOBBCollider&>(b), wtB), wa);
        case ColliderType::kPoint:  return intersect(wa, buildWorldPoint(static_cast<const PointCollider&>(b), wtB));
        case ColliderType::kLine:   return intersect(wa, buildWorldLine(static_cast<const LineCollider&>(b), wtB));
      }
      break;
    }
    case ColliderType::kLine: {
      const auto& ca = static_cast<const LineCollider&>(a);
      const WorldLine wa = buildWorldLine(ca, wtA);
      switch (b.getType()) {
        case ColliderType::kCircle:
          return intersect(buildWorldCircle(static_cast<const CircleCollider&>(b), wtB), wa);
        case ColliderType::kAABB:
          return intersect(buildWorldAABB(static_cast<const AABBCollider&>(b), wtB), wa);
        case ColliderType::kOOBB:
          return intersect(buildWorldOOBB(static_cast<const OOBBCollider&>(b), wtB), wa);
        case ColliderType::kPoint:
          return intersect(buildWorldPoint(static_cast<const PointCollider&>(b), wtB), wa);
        case ColliderType::kLine:   return intersect(wa, buildWorldLine(static_cast<const LineCollider&>(b), wtB));
      }
      break;
    }
  }
  return {};
}

} // namespace sfmx
