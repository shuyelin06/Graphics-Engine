#include "GJKSupport.h"

namespace Engine
{
namespace Math
{
GJKSupportPointSet::GJKSupportPointSet() = default;
GJKSupportPointSet::~GJKSupportPointSet() = default;

// AddPoint:
// Add a point to the set
void GJKSupportPointSet::addPoint(const Vector3& point) {
    points.push_back(point);
}

// Clear:
// Removes all points in the point set
void GJKSupportPointSet::reset() {
    points.clear();
}

// Center:
// Calculates the center of the set
const Vector3 GJKSupportPointSet::center(void) {
    Vector3 center = Vector3(0,0,0);
    
    for (const Vector3& point : points) {
        center += point;
    }

    center /= points.size();

    return center;
}

// FurthestPoint:
// Given a directional vector, calculates the point furthest in that
// direction
const Vector3 GJKSupportPointSet::furthestPoint(const Vector3& direction) {
    if (points.size() == 0) {
        return Vector3(0,0,0);
    } 

    const Vector3 direc = direction.unit();
    Vector3 furthest = points[0];

    for (const Vector3& point : points) {
        if (point.dot(direc) >= furthest.dot(direc)) {
            furthest = point;
        }
    }

    return furthest;
}

}}