#pragma once

// This file contains various implementations of GJKSupport
// classes, which can all be used as supporting functions for the
// GJK algorithm given in GJK.cpp.

#include <vector>

#include "math/Transform.h"
#include "math/Vector3.h"

namespace Engine {
using namespace Math;

namespace Physics {
class GJKSupportFunc {
  public:
    virtual const Vector3 center() const = 0;
    virtual const Vector3 furthestPoint(const Vector3& direction) const = 0;
};

// GJKSupportPointSet Class:
// Support function that takes a point set, and returns the point of the convex
// hull in that point set.
class GJKSupportPointSet : public GJKSupportFunc {
  private:
    std::vector<Vector3> points;
    
    const Transform* transform;

  public:
    GJKSupportPointSet(const Transform* transform);
    ~GJKSupportPointSet();

    const std::vector<Vector3>& getPoints();
    
    void setTransform(const Transform* transform);
    void addPoint(const Vector3& point);
    
    void reset();

    const Vector3 center(void) const;
    const Vector3 furthestPoint(const Vector3& direction) const;
};

} // namespace Math
} // namespace Engine