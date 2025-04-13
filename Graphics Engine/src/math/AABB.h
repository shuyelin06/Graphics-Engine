#pragma once

#include "Vector3.h"

namespace Engine {
namespace Math {
// Axis-Aligned Bounding Box (AABB):
// Represents an AABB in 3D space.
class AABB {
  private:
    Vector3 minimum;
    Vector3 maximum;

  public:
    AABB();

    // Return information about the AABB
    float volume() const;
    float area() const;

    const Vector3& getMin() const;
    const Vector3& getMax() const;

    // AABB Operations
    AABB unionWith(const AABB& aabb) const;

    // Populate an 8+ Vector3 array with the AABB points
    void fillArrWithPoints(Vector3* point_arr) const;

    // Modify the AABB
    void expandToContain(const Vector3& point);
};

} // namespace Math
} // namespace Engine