#include "AABB.h"

#include <math.h>

namespace Engine {
namespace Math {
AABB::AABB() : minimum(Vector3::VectorMax()), maximum(Vector3::VectorMin()) {}

// Getters:
// Return properties of the AABB.
float AABB::volume() const {
    const Vector3 difference = maximum - minimum;
    return fabsf(difference.x * difference.y * difference.z);
}
float AABB::area() const {
    const Vector3 diff = maximum - minimum;
    return 2 * (diff.x * diff.y + diff.x * diff.z + diff.y * diff.z);
}
const Vector3& AABB::getMin() const { return minimum; }
const Vector3& AABB::getMax() const { return maximum; }

// PopulatePointArray:
// Populate an 8+ Vector3 array with the AABB points.
// Array must be size 8 or more.
void AABB::fillArrWithPoints(Vector3* point_arr) const {
    point_arr[0] = Vector3(minimum.x, minimum.y, minimum.z);
    point_arr[1] = Vector3(maximum.x, minimum.y, minimum.z);
    point_arr[2] = Vector3(maximum.x, maximum.y, minimum.z);
    point_arr[3] = Vector3(minimum.x, maximum.y, minimum.z);

    point_arr[4] = Vector3(minimum.x, minimum.y, maximum.z);
    point_arr[5] = Vector3(maximum.x, minimum.y, maximum.z);
    point_arr[6] = Vector3(maximum.x, maximum.y, maximum.z);
    point_arr[7] = Vector3(minimum.x, maximum.y, maximum.z);
}

// ExpandToContain:
// Given a point, expands the AABB so that it includes the point.
void AABB::expandToContain(const Vector3& point) {
    minimum = minimum.componentMin(point);
    maximum = maximum.componentMax(point);
}

} // namespace Math
} // namespace Engine