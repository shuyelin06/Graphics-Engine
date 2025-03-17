#include "OBB.h"

namespace Engine {
namespace Math {
OBB::OBB(const Vector3& center, const Quaternion& rotation) : aabb() {
    m_local_to_world =
        Matrix4::T_Translate(center) * rotation.rotationMatrix4();
}

OBB::OBB(const AABB& _aabb, const Matrix4& m_local)
    : aabb(_aabb), m_local_to_world(m_local) {}

// Getters:
// Return properties of the OBB
const AABB& OBB::getAABB() const { return aabb; }

Vector3 OBB::getCenter() const {
    const Vector3 center = m_local_to_world.column(3).xyz();
    return center;
}

Vector3 OBB::axis1() const {
    const Vector3 axis = m_local_to_world.column(0).xyz();
    return axis.unit();
}
Vector3 OBB::axis2() const {
    const Vector3 axis = m_local_to_world.column(1).xyz();
    return axis.unit();
}
Vector3 OBB::axis3() const {
    const Vector3 axis = m_local_to_world.column(2).xyz();
    return axis.unit();
}

// PopulateAxisArray:
// Populate a 3+ size array with the axes of the OBB
void OBB::fillArrWithAxes(Vector3* axis_arr) const {
    axis_arr[0] = axis1();
    axis_arr[1] = axis2();
    axis_arr[2] = axis3();
}

// PopulatePointArray:
// Populate a 8+ size array with the points of the OBB
void OBB::fillArrWithPoints(Vector3* point_arr) const {
    // Populate the array with the AABB points
    aabb.fillArrWithPoints(point_arr);

    // Transform the AABB points from their local space to
    // world space, using the OBB's rotation and center.
    for (int i = 0; i < 8; i++) {
        const Vector4 point = Vector4(point_arr[i], 1.f);
        point_arr[i] = (m_local_to_world * point).xyz();
    }
}

// ExpandToContain:
// Expand the OBB to contain the following point
void OBB::expandToContain(const Vector3* points, int num_points) {
    const Matrix4 m_inverse = m_local_to_world.inverse();

    for (int i = 0; i < num_points; i++) {
        const Vector3 point_local = (m_inverse * Vector4(points[i], 1.f)).xyz();
        aabb.expandToContain(point_local);
    }
}

void OBB::expandToContain(const Vector3& point) {
    // Translate the point into the OBB's local space
    const Matrix4 m_inverse = m_local_to_world.inverse();

    const Vector3 point_local = (m_inverse * Vector4(point, 1.f)).xyz();
    aabb.expandToContain(point_local);
}

} // namespace Math
} // namespace Engine