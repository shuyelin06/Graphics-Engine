#pragma once

#include "../Vector3.h"

namespace Engine {
namespace Math {
class GJKSupportFunc;
struct GJKSimplex;

// GJKSolver Class:
// Implements the GJK algorithm. Takes two support functions, and returns
// whether or not the shapes represented by the support functions are
// intersections. Makes a few key assumptions:
// 1) The support function is a function which, given a direction, returns the
//    FURTHEST point of its underlying shape in that direction.
// 2) The underlying shape must be convex.
// Based on
// https://blog.hamaluik.ca/posts/building-a-collision-engine-part-3-3d-gjk-collision-detection/
enum SolverStatus { IntersectionFalse, IntersectionTrue, Evolving };
class GJKSolver {
  private:
    GJKSupportFunc* shape_1;
    GJKSupportFunc* shape_2;

    // Helper simplex used to find collisions and their information
    GJKSimplex* simplex;
    Vector3 direction;

  public:
    GJKSolver(GJKSupportFunc* shape_1, GJKSupportFunc* shape_2);

    // Returns if the two shapes are intersecting or not
    bool checkIntersection();

    // If the two shapes are intersecting, returns the penetration vector
    Vector3 penetrationVector();

  private:
    // Performs 1 iteration of the GJK algorithm
    SolverStatus iterate(); 

    const Vector3 querySupports(const Vector3& direction);
};

} // namespace Math
} // namespace Engine