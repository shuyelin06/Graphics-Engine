#pragma once

// This file contains various implementations of GJKSupport
// classes, which can all be used as supporting functions for the
// GJK algorithm given in GJK.cpp.

#include <vector>

#include "../Vector3.h"

namespace Engine {
namespace Math {
class GJKSupportFunc {
  public:
    virtual const Vector3 center() = 0;
    virtual const Vector3 furthestPoint(const Vector3& direction) = 0;
};

// GJKSupportPointSet Class:
// Support function that takes a point set, and returns the point of the convex
// hull in that point set.
class GJKSupportPointSet : public GJKSupportFunc {
  private:
    std::vector<Vector3> points;

  public:
    GJKSupportPointSet();
    ~GJKSupportPointSet();

    void addPoint(const Vector3& point);
    void reset();

    const Vector3 center(void);
    const Vector3 furthestPoint(const Vector3& direction);
};

} // namespace Math
} // namespace Engine