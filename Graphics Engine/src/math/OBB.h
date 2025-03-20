#pragma once

#include "AABB.h"
#include "Matrix4.h"

#include "Quaternion.h"
#include "Vector3.h"

namespace Engine {
namespace Math {
// Orientated Bounding-Box (OBB):
// Represents an OBB in 3D space, which is an AABB centered around a point
// and rotation.
class OBB {
  private:
    AABB aabb;
    Matrix4 m_local_to_world;

  public:
    OBB(const Vector3& center, const Quaternion& rotation);
    OBB(const AABB& aabb, const Matrix4& m_local);

    // Get properties of the OBB
    const AABB& getAABB() const;
    
    Vector3 getCenter() const;
    Vector3 axis1() const;
    Vector3 axis2() const;
    Vector3 axis3() const;

    // Populate a 3+ size array with the axes of the OBB
    void fillArrWithAxes(Vector3* axis_arr) const;
    // Populate a 8+ size array with the points of the OBB
    void fillArrWithPoints(Vector3* point_arr) const;

    // Update the properties of the OBB
    void expandToContain(const Vector3* points, int num_points);
    void expandToContain(const Vector3& point);

};

} // namespace Math
} // namespace Engine