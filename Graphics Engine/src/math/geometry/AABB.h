#pragma once

#include "../Vector3.h"

#if defined(_DEBUG)
#define DRAW_AABB_EXTENTS
#endif

namespace Engine {
namespace Math {
// AxisAlignedBoundingBox (AABB):
// Represents an AABB in 3D space, given by its lower left corner and upper
// right corner.
class AABB {
  private:
    Vector3 minimum;
    Vector3 maximum;

  public:
    AABB();
    AABB(const Vector3& center);
    ~AABB();

    float volume() const;

    const Vector3& getMin() const;
    const Vector3& getMax() const;

    void expandToContain(const Vector3& point);

#if defined(DRAW_AABB_EXTENTS)
    void debugDrawExtents() const;
#endif
};

} // namespace Math
} // namespace Engine