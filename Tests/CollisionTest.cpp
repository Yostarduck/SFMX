#include <doctest/doctest.h>

#include "core/physics/Collider.h"
#include "core/platform/Prerequisites.h"

#include <SFML/Graphics/Transform.hpp>

using namespace sfmx;

namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Identity transform for local-space tests */
sf::Transform id;

/** Quick epsilon check */
bool approx(float a, float b, float eps = 1e-4f) {
  return std::abs(a - b) < eps;
}

bool approx(sf::Vector2f v, sf::Vector2f e, float eps = 1e-4f) {
  return approx(v.x, e.x, eps) && approx(v.y, e.y, eps);
}

// ---------------------------------------------------------------------------
// Circle vs Circle
// ---------------------------------------------------------------------------

TEST_CASE("Collision - Circle vs Circle - hit") {
  CircleCollider a({0.f, 0.f}, 5.f);
  CircleCollider b({6.f, 0.f}, 5.f);
  auto r = intersect(a, id, b, id);
  CHECK(r.hit);
  CHECK(approx(r.penetration, 4.f));
  CHECK(approx(r.normal.x, 1.f)); // normal points from a toward b
}

TEST_CASE("Collision - Circle vs Circle - miss") {
  CircleCollider a({0.f, 0.f}, 5.f);
  CircleCollider b({12.f, 0.f}, 5.f);
  CHECK_FALSE(intersect(a, id, b, id).hit);
}

TEST_CASE("Collision - Circle vs Circle - concentric") {
  CircleCollider a({0.f, 0.f}, 5.f);
  CircleCollider b({0.f, 0.f}, 3.f);
  auto r = intersect(a, id, b, id);
  CHECK(r.hit);
  CHECK(approx(r.penetration, 8.f));
}

// ---------------------------------------------------------------------------
// Circle vs AABB
// ---------------------------------------------------------------------------

TEST_CASE("Collision - Circle vs AABB - hit") {
  CircleCollider circle({0.f, 0.f}, 5.f);
  AABBCollider   aabb({8.f, 0.f}, {4.f, 4.f}); // extends from (4,0) to (12,0), circle radius 5 reaches
  auto r = intersect(circle, id, aabb, id);
  CHECK(r.hit);
  CHECK(r.penetration > 0.f);
}

TEST_CASE("Collision - Circle vs AABB - miss") {
  CircleCollider circle({0.f, 0.f}, 5.f);
  AABBCollider   aabb({20.f, 0.f}, {4.f, 4.f});
  CHECK_FALSE(intersect(circle, id, aabb, id).hit);
}

// ---------------------------------------------------------------------------
// Circle vs OBB
// ---------------------------------------------------------------------------

TEST_CASE("Collision - Circle vs OBB - hit") {
  CircleCollider circle({0.f, 0.f}, 5.f);
  OBBCollider    obb({8.f, 0.f}, {4.f, 4.f});
  auto r = intersect(circle, id, obb, id);
  CHECK(r.hit);
}

TEST_CASE("Collision - Circle vs OBB - miss") {
  CircleCollider circle({0.f, 0.f}, 5.f);
  OBBCollider    obb({20.f, 0.f}, {4.f, 4.f});
  CHECK_FALSE(intersect(circle, id, obb, id).hit);
}

// ---------------------------------------------------------------------------
// Circle vs Point
// ---------------------------------------------------------------------------

TEST_CASE("Collision - Circle vs Point - inside") {
  CircleCollider circle({0.f, 0.f}, 5.f);
  PointCollider  point({3.f, 0.f});
  CHECK(intersect(circle, id, point, id).hit);
}

TEST_CASE("Collision - Circle vs Point - outside") {
  CircleCollider circle({0.f, 0.f}, 5.f);
  PointCollider  point({10.f, 0.f});
  CHECK_FALSE(intersect(circle, id, point, id).hit);
}

// ---------------------------------------------------------------------------
// Circle vs Line
// ---------------------------------------------------------------------------

TEST_CASE("Collision - Circle vs Line - hit") {
  CircleCollider circle({0.f, 0.f}, 5.f);
  LineCollider   line({-2.f, -5.f}, {2.f, 5.f});
  CHECK(intersect(circle, id, line, id).hit);
}

TEST_CASE("Collision - Circle vs Line - miss") {
  CircleCollider circle({0.f, 0.f}, 5.f);
  LineCollider   line({-2.f, 10.f}, {2.f, 12.f});
  CHECK_FALSE(intersect(circle, id, line, id).hit);
}

// ---------------------------------------------------------------------------
// AABB vs AABB
// ---------------------------------------------------------------------------

TEST_CASE("Collision - AABB vs AABB - hit") {
  AABBCollider a({0.f, 0.f}, {5.f, 5.f});
  AABBCollider b({6.f, 0.f}, {5.f, 5.f});
  auto r = intersect(a, id, b, id);
  CHECK(r.hit);
  CHECK(r.penetration > 0.f);
}

TEST_CASE("Collision - AABB vs AABB - miss") {
  AABBCollider a({0.f, 0.f}, {5.f, 5.f});
  AABBCollider b({15.f, 0.f}, {5.f, 5.f});
  CHECK_FALSE(intersect(a, id, b, id).hit);
}

// ---------------------------------------------------------------------------
// AABB vs OBB (delegates to SAT via worldOBBFromAABB)
// ---------------------------------------------------------------------------

TEST_CASE("Collision - AABB vs OBB - hit") {
  AABBCollider aabb({0.f, 0.f}, {5.f, 5.f});
  OBBCollider  obb({6.f, 0.f}, {4.f, 4.f});
  CHECK(intersect(aabb, id, obb, id).hit);
}

// ---------------------------------------------------------------------------
// AABB vs Point
// ---------------------------------------------------------------------------

TEST_CASE("Collision - AABB vs Point - inside") {
  AABBCollider aabb({0.f, 0.f}, {5.f, 5.f});
  PointCollider point({2.f, 2.f});
  CHECK(intersect(aabb, id, point, id).hit);
}

TEST_CASE("Collision - AABB vs Point - outside") {
  AABBCollider aabb({0.f, 0.f}, {5.f, 5.f});
  PointCollider point({10.f, 0.f});
  CHECK_FALSE(intersect(aabb, id, point, id).hit);
}

// ---------------------------------------------------------------------------
// AABB vs Line
// ---------------------------------------------------------------------------

TEST_CASE("Collision - AABB vs Line - endpoint inside") {
  AABBCollider aabb({0.f, 0.f}, {5.f, 5.f});
  LineCollider line({-10.f, 0.f}, {2.f, 2.f});
  CHECK(intersect(aabb, id, line, id).hit);
}

TEST_CASE("Collision - AABB vs Line - segment through") {
  AABBCollider aabb({0.f, 0.f}, {5.f, 5.f});
  LineCollider line({-5.f, -3.f}, {5.f, 3.f});
  CHECK(intersect(aabb, id, line, id).hit);
}

// ---------------------------------------------------------------------------
// OBB vs OBB (SAT)
// ---------------------------------------------------------------------------

TEST_CASE("Collision - OBB vs OBB - hit") {
  OBBCollider a({0.f, 0.f}, {4.f, 4.f});
  OBBCollider b({5.f, 0.f}, {4.f, 4.f});
  CHECK(intersect(a, id, b, id).hit);
}

TEST_CASE("Collision - OBB vs OBB - miss") {
  OBBCollider a({0.f, 0.f}, {4.f, 4.f});
  OBBCollider b({15.f, 0.f}, {4.f, 4.f});
  CHECK_FALSE(intersect(a, id, b, id).hit);
}

// ---------------------------------------------------------------------------
// OBB vs Point
// ---------------------------------------------------------------------------

TEST_CASE("Collision - OBB vs Point - inside") {
  OBBCollider  obb({0.f, 0.f}, {5.f, 5.f});
  PointCollider point({2.f, 2.f});
  CHECK(intersect(obb, id, point, id).hit);
}

TEST_CASE("Collision - OBB vs Point - outside") {
  OBBCollider  obb({0.f, 0.f}, {5.f, 5.f});
  PointCollider point({10.f, 0.f});
  CHECK_FALSE(intersect(obb, id, point, id).hit);
}

// ---------------------------------------------------------------------------
// OBB vs Line
// ---------------------------------------------------------------------------

TEST_CASE("Collision - OBB vs Line - endpoint inside") {
  OBBCollider  obb({0.f, 0.f}, {5.f, 5.f});
  LineCollider line({-10.f, 0.f}, {2.f, 2.f});
  CHECK(intersect(obb, id, line, id).hit);
}

// ---------------------------------------------------------------------------
// Point vs Point
// ---------------------------------------------------------------------------

TEST_CASE("Collision - Point vs Point - exact") {
  PointCollider a({3.f, 4.f});
  PointCollider b({3.f, 4.f});
  CHECK(intersect(a, id, b, id).hit);
}

TEST_CASE("Collision - Point vs Point - miss") {
  PointCollider a({0.f, 0.f});
  PointCollider b({5.f, 0.f});
  CHECK_FALSE(intersect(a, id, b, id).hit);
}

// ---------------------------------------------------------------------------
// Point vs Line
// ---------------------------------------------------------------------------

TEST_CASE("Collision - Point vs Line - on segment") {
  PointCollider point({0.f, 0.f});
  LineCollider  line({-5.f, 0.f}, {5.f, 0.f});
  CHECK(intersect(point, id, line, id).hit);
}

TEST_CASE("Collision - Point vs Line - off segment") {
  PointCollider point({10.f, 0.f});
  LineCollider  line({-5.f, 0.f}, {5.f, 0.f});
  CHECK_FALSE(intersect(point, id, line, id).hit);
}

// ---------------------------------------------------------------------------
// Line vs Line
// ---------------------------------------------------------------------------

TEST_CASE("Collision - Line vs Line - crossing") {
  LineCollider a({-5.f,  0.f}, {5.f, 0.f});
  LineCollider b({ 0.f, -5.f}, {0.f, 5.f});
  CHECK(intersect(a, id, b, id).hit);
}

TEST_CASE("Collision - Line vs Line - parallel") {
  LineCollider a({-5.f, 0.f}, {5.f,  0.f});
  LineCollider b({-5.f, 2.f}, {5.f,  2.f});
  CHECK_FALSE(intersect(a, id, b, id).hit);
}

// ---------------------------------------------------------------------------
// Public dispatch via ICollider base (cover the TypeTraits-based branches)
// ---------------------------------------------------------------------------

TEST_CASE("Collision - public dispatch covers all type pairs") {
  CircleCollider circle({0.f, 0.f}, 5.f);
  AABBCollider   aabb({4.f, 0.f}, {4.f, 4.f});  // extends (-0,4) to (8,4) — overlaps circle
  OBBCollider    obb({4.f, 0.f}, {4.f, 4.f});   // same region via OBB
  PointCollider  pt({2.f, 0.f});                 // inside circle
  LineCollider   line({-3.f, 0.f}, {3.f, 0.f}); // through circle

  // Circle as A, each type as B
  CHECK(intersect(static_cast<const ICollider&>(circle), id,
                  static_cast<const ICollider&>(circle), id).hit);
  CHECK(intersect(static_cast<const ICollider&>(circle), id,
                  static_cast<const ICollider&>(aabb), id).hit);
  CHECK(intersect(static_cast<const ICollider&>(circle), id,
                  static_cast<const ICollider&>(obb), id).hit);
  CHECK(intersect(static_cast<const ICollider&>(circle), id,
                  static_cast<const ICollider&>(pt), id).hit);
  CHECK(intersect(static_cast<const ICollider&>(circle), id,
                  static_cast<const ICollider&>(line), id).hit);

  // AABB as A, each type as B
  CHECK(intersect(static_cast<const ICollider&>(aabb), id,
                  static_cast<const ICollider&>(circle), id).hit);
  CHECK(intersect(static_cast<const ICollider&>(aabb), id,
                  static_cast<const ICollider&>(aabb), id).hit);
  CHECK(intersect(static_cast<const ICollider&>(aabb), id,
                  static_cast<const ICollider&>(obb), id).hit);
  CHECK(intersect(static_cast<const ICollider&>(aabb), id,
                  static_cast<const ICollider&>(pt), id).hit);
  CHECK(intersect(static_cast<const ICollider&>(aabb), id,
                  static_cast<const ICollider&>(line), id).hit);

  // OBB as A, each type as B
  CHECK(intersect(static_cast<const ICollider&>(obb), id,
                  static_cast<const ICollider&>(circle), id).hit);
  CHECK(intersect(static_cast<const ICollider&>(obb), id,
                  static_cast<const ICollider&>(aabb), id).hit);
  CHECK(intersect(static_cast<const ICollider&>(obb), id,
                  static_cast<const ICollider&>(obb), id).hit);
  CHECK(intersect(static_cast<const ICollider&>(obb), id,
                  static_cast<const ICollider&>(pt), id).hit);
  CHECK(intersect(static_cast<const ICollider&>(obb), id,
                  static_cast<const ICollider&>(line), id).hit);
}

// ---------------------------------------------------------------------------
// Transform-dependent tests
// ---------------------------------------------------------------------------

TEST_CASE("Collision - translated Circle vs Circle") {
  CircleCollider a({0.f, 0.f}, 5.f);
  CircleCollider b({0.f, 0.f}, 5.f);
  // b shifted right by 6 via transform
  sf::Transform t; t.translate({6.f, 0.f});
  auto r = intersect(a, id, b, t);
  CHECK(r.hit);
  CHECK(approx(r.penetration, 4.f));
}

// ---------------------------------------------------------------------------
// Type traits verification
// ---------------------------------------------------------------------------

TEST_CASE("Collision - type IDs are unique") {
  CHECK(TypeTraits<CircleCollider>::getTypeId() !=
        TypeTraits<AABBCollider>::getTypeId());
  CHECK(TypeTraits<CircleCollider>::getTypeId() !=
        TypeTraits<OBBCollider>::getTypeId());
  CHECK(TypeTraits<CircleCollider>::getTypeId() !=
        TypeTraits<PointCollider>::getTypeId());
  CHECK(TypeTraits<CircleCollider>::getTypeId() !=
        TypeTraits<LineCollider>::getTypeId());
}

TEST_CASE("Collision - getTypeId is consistent per type") {
  CircleCollider a, b;
  CHECK(a.getTypeId() == b.getTypeId());
  CHECK(a.getTypeId() == TypeTraits<CircleCollider>::getTypeId());

  AABBCollider aabb;
  CHECK(aabb.getTypeId() == TypeTraits<AABBCollider>::getTypeId());
}

// ---------------------------------------------------------------------------
// CollisionResult operator bool
// ---------------------------------------------------------------------------

TEST_CASE("Collision - CollisionResult operator bool") {
  CollisionResult hit;  hit.hit = true;
  CollisionResult miss;
  CHECK(hit);
  CHECK_FALSE(miss);
}

} // namespace
