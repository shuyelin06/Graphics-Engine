#pragma once

#include "Vector3.h"

namespace Engine {
namespace Math {
// GJKSolver Class:
// Implements the GJK algorithm. Takes two support functions, and returns
// whether or not the shapes represented by the support functions are
// intersections. Makes a few key assumptions:
// 1) The support function is a function which, given a direction, returns the
//    FURTHEST point of its underlying shape in that direction.
// 2) The underlying shape must be convex.
// Based on
// https://blog.hamaluik.ca/posts/building-a-collision-engine-part-3-3d-gjk-collision-detection/
class GJKSupportFunc {
  public:
    virtual const Vector3 center() = 0;
    virtual const Vector3 furthestPoint(const Vector3& direction) = 0;
};

class GJKSimplex {
  private:
    Vector3 points[4];
    int num_points;

  public:
    GJKSimplex();

    void push_back(const Vector3& p);
    void remove(int index);
    int size() const;

    // Last (p1) to first (p4) vertex inserted, in that order
    const Vector3 p1() const;
    const Vector3 p2() const;
    const Vector3 p3() const;
    const Vector3 p4() const;
};

enum SolverStatus { IntersectionFalse, IntersectionTrue, Evolving };
class GJKSolver {
  private:
    GJKSupportFunc* shape_1;
    GJKSupportFunc* shape_2;

    GJKSimplex simplex;
    Vector3 direction;

  public:
    GJKSolver(GJKSupportFunc* shape_1, GJKSupportFunc* shape_2);

    bool checkIntersection();

  private:
    SolverStatus iterate();
    const Vector3 querySupports(const Vector3& direction);
};

} // namespace Math
} // namespace Engine