#include "Frustum.h"

#include <algorithm>
#include <float.h>
#include <math.h>

namespace Engine {
namespace Graphics {
Frustum::Frustum(const Matrix4& _m_world_to_frustum) {
    m_world_to_frustum = _m_world_to_frustum;
    m_frustum_to_world = m_world_to_frustum.inverse();
}

Vector3 Frustum::toWorldSpace(const Vector3& frustum_coords) const {
    Vector4 transformed = m_frustum_to_world * Vector4(frustum_coords, 1.f);
    transformed = transformed / transformed.w;
    return transformed.xyz();
}

Vector3 Frustum::toFrustumSpace(const Vector3& world_space) const {
    Vector4 transformed = m_world_to_frustum * Vector4(world_space, 1.f);
    transformed = transformed / transformed.w;
    return transformed.xyz();
}

// FillArrWithFrustumPoints:
// Fills a size 8+ array with the frustum points in frustum space.
void Frustum::fillArrWithFrustumPoints(Vector3* point_arr) const {
    // Near Plane
    point_arr[0] = Vector3(-1, -1, 0);
    point_arr[1] = Vector3(1, -1, 0);
    point_arr[2] = Vector3(1, 1, 0);
    point_arr[3] = Vector3(-1, 1, 0);

    // Far Plane
    point_arr[4] = Vector3(-1, -1, 1);
    point_arr[5] = Vector3(1, -1, 1);
    point_arr[6] = Vector3(1, 1, 1);
    point_arr[7] = Vector3(-1, 1, 1);
}

// fillArrWithWorldPoints:
// Fills a size 8+ array with the frustum points in world space.
void Frustum::fillArrWithWorldPoints(Vector3* point_arr) const {
    fillArrWithFrustumPoints(point_arr);

    // Transform to world space
    for (int i = 0; i < 8; i++) {
        const Vector3 point = point_arr[i];
        point_arr[i] = toWorldSpace(point);
    }
}

// IntersectsAABB:
// Performs the Separating Axis Theorem (SAT) to figure out if the frustum
// intersects an AABB. Useful in frustum culling.
static bool testSeparationAlongAxis(const Vector3& axis,
                                    const Vector3* frustum_points,
                                    const Vector3* obb_points);

bool Frustum::intersectsOBB(const OBB& obb) const {
    // Fill my arrays with the OBB and frustum points, both in world
    // coordinates
    Vector3 frust_pts[8];
    Vector3 obb_points[8];

    fillArrWithWorldPoints(frust_pts);
    obb.fillArrWithPoints(obb_points);

    // Now, for 26 different axes, check for separation. If there is separation,
    // we can early out. Otherwise, we only know they intersect one another
    // if all axes have an overlap.
    Vector3 axis;

    // Case 1 - 5: Frustum Axes
    axis = (frust_pts[1] - frust_pts[0]).cross(frust_pts[3] - frust_pts[0]);
    if (testSeparationAlongAxis(axis, frust_pts, obb_points))
        return false;

    axis = (frust_pts[5] - frust_pts[1]).cross(frust_pts[2] - frust_pts[1]);
    if (testSeparationAlongAxis(axis, frust_pts, obb_points))
        return false;

    axis = (frust_pts[6] - frust_pts[2]).cross(frust_pts[3] - frust_pts[2]);
    if (testSeparationAlongAxis(axis, frust_pts, obb_points))
        return false;

    axis = (frust_pts[7] - frust_pts[3]).cross(frust_pts[0] - frust_pts[3]);
    if (testSeparationAlongAxis(axis, frust_pts, obb_points))
        return false;

    axis = (frust_pts[4] - frust_pts[0]).cross(frust_pts[1] - frust_pts[0]);
    if (testSeparationAlongAxis(axis, frust_pts, obb_points))
        return false;

    // Case 6 - 26: OBB Axes
    Vector3 obb_axes[3];
    obb.fillArrWithAxes(obb_axes);

    for (const Vector3& obb_axis : obb_axes) {
        axis = obb_axis;
        if (testSeparationAlongAxis(axis, frust_pts, obb_points))
            return false;

        axis = obb_axis.cross(frust_pts[1] - frust_pts[0]);
        if (testSeparationAlongAxis(axis, frust_pts, obb_points))
            return false;

        axis = obb_axis.cross(frust_pts[3] - frust_pts[0]);
        if (testSeparationAlongAxis(axis, frust_pts, obb_points))
            return false;

        axis = obb_axis.cross(frust_pts[5] - frust_pts[1]);
        if (testSeparationAlongAxis(axis, frust_pts, obb_points))
            return false;

        axis = obb_axis.cross(frust_pts[6] - frust_pts[2]);
        if (testSeparationAlongAxis(axis, frust_pts, obb_points))
            return false;

        axis = obb_axis.cross(frust_pts[7] - frust_pts[3]);
        if (testSeparationAlongAxis(axis, frust_pts, obb_points))
            return false;

        axis = obb_axis.cross(frust_pts[4] - frust_pts[0]);
        if (testSeparationAlongAxis(axis, frust_pts, obb_points))
            return false;
    }

    return true;
}

bool testSeparationAlongAxis(const Vector3& _axis,
                             const Vector3* frustum_points,
                             const Vector3* obb_points) {
    if (_axis.magnitude() < 0.0001f)
        return false;

    const Vector3 axis = _axis.unit();

    // Project both points to the axis, and see if the intervals of projections
    // overlap.
    // - If they overlap, then we have to test another axis.
    // - If they don't overlap, then we have a separating axis and by the SAT
    // the frustum
    //   and OBB cannot overlap.
    float frustum_min = FLT_MAX, frustum_max = FLT_MIN;
    float obb_min = FLT_MAX, obb_max = FLT_MIN;

    for (int i = 0; i < 8; i++) {
        // Project frustum points
        const Vector3& frustum_point = frustum_points[i];
        const float frustum_projection = frustum_point.scalarProjection(axis);
        frustum_min = std::min(frustum_min, frustum_projection);
        frustum_max = std::max(frustum_max, frustum_projection);

        // Project obb points
        const Vector3& obb_point = obb_points[i];
        const float obb_projection = obb_point.scalarProjection(axis);
        obb_min = std::min(obb_min, obb_projection);
        obb_max = std::max(obb_max, obb_projection);
    }

    // See if the projection intervals are separate. If they are, return true.
    if (obb_max < frustum_min || frustum_max < obb_min)
        return true;
    else
        return false;
}
} // namespace Graphics
} // namespace Engine