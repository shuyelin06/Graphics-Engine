#include "CollisionAABB.h"

#include <math.h>

#if defined(DRAW_AABB_EXTENTS)
#include "rendering/VisualDebug.h"
#endif

namespace Engine {
namespace Physics {
CollisionAABB::CollisionAABB() : minimum(Vector3::VectorMax()), maximum(Vector3::VectorMin()) {
    node = nullptr;
}
CollisionAABB::CollisionAABB(const Vector3& center)
    : minimum(center), maximum(center), node(nullptr) {}
CollisionAABB::~CollisionAABB() = default;

// Getters:
// Return properties of the AABB.
float CollisionAABB::volume() const {
    const Vector3 difference = maximum - minimum;
    return fabsf(difference.x * difference.y * difference.z);
}
const Vector3& CollisionAABB::getMin() const { return minimum; }
const Vector3& CollisionAABB::getMax() const { return maximum; }

// Contains:
// Returns true if this AABB contains the parameter, false otherwise
bool CollisionAABB::contains(const CollisionAABB& aabb) const {
    const bool x = minimum.x <= aabb.minimum.x && aabb.maximum.x <= maximum.x;
    const bool y = minimum.y <= aabb.minimum.y && aabb.maximum.y <= maximum.y;
    const bool z = minimum.z <= aabb.minimum.z && aabb.maximum.z <= maximum.z;
    return x && y && z;
}

bool CollisionAABB::contains(const Vector3& point) const {
    const bool x = minimum.x <= point.x && point.x <= maximum.x;
    const bool y = minimum.y <= point.y && point.y <= maximum.y;
    const bool z = minimum.z <= point.z && point.z <= maximum.z;
    return x && y && z;
}

// Intersects:
// Returns true if the two AABBs are intersecting. Determines this using the
// separating axis theorem, which asserts that the AABBS cannot intersect if
// there exists an axis separating them.
bool CollisionAABB::intersects(const CollisionAABB& aabb) const {
    // Check x-axis
    if (maximum.x < aabb.minimum.x || aabb.maximum.x < minimum.x)
        return false;

    // Check y-axis
    if (maximum.y < aabb.minimum.y || aabb.maximum.y < minimum.y)
        return false;

    // Check z-axis
    if (maximum.z < aabb.minimum.z || aabb.maximum.z < minimum.z)
        return false;

    // Intersects on all axes, the AABBs are intersecting
    return true;
}

// UnionWith:
// Returns the union of two AABBs
CollisionAABB CollisionAABB::unionWith(const CollisionAABB& aabb) const {
    CollisionAABB result;
    result.minimum = minimum.componentMin(aabb.minimum);
    result.maximum = maximum.componentMax(aabb.maximum);
    return result;
}

// ExpandToContain:
// Given a point, expands the AABB so that it includes the point.
void CollisionAABB::expandToContain(const std::vector<Vector3>& points) {
    for (const Vector3& point : points)
        expandToContain(point);
}

void CollisionAABB::expandToContain(const Vector3& point) {
    minimum = minimum.componentMin(point);
    maximum = maximum.componentMax(point);
}

void CollisionAABB::reset() {
    minimum = Vector3::VectorMax();
    maximum = Vector3::VectorMin();
}

#if defined(DRAW_AABB_EXTENTS)
void CollisionAABB::debugDrawExtents() const { debugDrawExtents(Color::Blue()); }

void CollisionAABB::debugDrawExtents(const Color& color) const {
    Graphics::VisualDebug::DrawLine(Vector3(minimum.x, minimum.y, minimum.z),
                                    Vector3(maximum.x, minimum.y, minimum.z),
                                    color);
    Graphics::VisualDebug::DrawLine(Vector3(maximum.x, minimum.y, minimum.z),
                                    Vector3(maximum.x, maximum.y, minimum.z),
                                    color);
    Graphics::VisualDebug::DrawLine(Vector3(maximum.x, maximum.y, minimum.z),
                                    Vector3(minimum.x, maximum.y, minimum.z),
                                    color);
    Graphics::VisualDebug::DrawLine(Vector3(minimum.x, maximum.y, minimum.z),
                                    Vector3(minimum.x, minimum.y, minimum.z),
                                    color);

    Graphics::VisualDebug::DrawLine(Vector3(minimum.x, minimum.y, maximum.z),
                                    Vector3(maximum.x, minimum.y, maximum.z),
                                    color);
    Graphics::VisualDebug::DrawLine(Vector3(maximum.x, minimum.y, maximum.z),
                                    Vector3(maximum.x, maximum.y, maximum.z),
                                    color);
    Graphics::VisualDebug::DrawLine(Vector3(maximum.x, maximum.y, maximum.z),
                                    Vector3(minimum.x, maximum.y, maximum.z),
                                    color);
    Graphics::VisualDebug::DrawLine(Vector3(minimum.x, maximum.y, maximum.z),
                                    Vector3(minimum.x, minimum.y, maximum.z),
                                    color);

    Graphics::VisualDebug::DrawLine(Vector3(minimum.x, minimum.y, minimum.z),
                                    Vector3(minimum.x, minimum.y, maximum.z),
                                    color);
    Graphics::VisualDebug::DrawLine(Vector3(minimum.x, maximum.y, minimum.z),
                                    Vector3(minimum.x, maximum.y, maximum.z),
                                    color);
    Graphics::VisualDebug::DrawLine(Vector3(maximum.x, minimum.y, minimum.z),
                                    Vector3(maximum.x, minimum.y, maximum.z),
                                    color);
    Graphics::VisualDebug::DrawLine(Vector3(maximum.x, maximum.y, minimum.z),
                                    Vector3(maximum.x, maximum.y, maximum.z),
                                    color);
}
#endif

} // namespace Physics
} // namespace Engine