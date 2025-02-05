#include "GJKSupport.h"

namespace Engine {
namespace Math {
GJKSupportPointSet::GJKSupportPointSet(const Transform* _transform) : points() {
    transform = _transform;
}
GJKSupportPointSet::~GJKSupportPointSet() = default;

// GetPoints:
// Returns the points of the point set
const std::vector<Vector3>& GJKSupportPointSet::getPoints() { return points; }

// SetTransform:
// Sets the transform of the point set to point to something.
void GJKSupportPointSet::setTransform(const Transform* _transform) {
    transform = _transform;
}

// AddPoint:
// Add a point to the set
void GJKSupportPointSet::addPoint(const Vector3& point) {
    points.push_back(point);
}

// Clear:
// Removes all points in the point set
void GJKSupportPointSet::reset() { points.clear(); }

// Center:
// Calculates the center of the set
const Vector3 GJKSupportPointSet::center(void) {
    Vector3 center = Vector3(0, 0, 0);

    for (const Vector3& point : points)
        center += point;
    center /= points.size();

    center += transform->getPosition();

    return center;
}

// FurthestPoint:
// Given a directional vector, calculates the point furthest in that
// direction
const Vector3 GJKSupportPointSet::furthestPoint(const Vector3& direction) {
    if (points.size() == 0) {
        return Vector3(0, 0, 0);
    }

    const Vector3 direc = direction.unit();
    const Matrix4 m_transform = transform->transformMatrix();

    Vector3 furthest = (m_transform * Vector4(points[0], 1.0f)).xyz();

    for (const Vector3& point : points) {
        const Vector3 transformed = (m_transform * Vector4(point, 1.0f)).xyz();
        if (transformed.dot(direc) >= furthest.dot(direc)) {
            furthest = transformed;
        }
    }

    return furthest;
}

} // namespace Math
} // namespace Engine