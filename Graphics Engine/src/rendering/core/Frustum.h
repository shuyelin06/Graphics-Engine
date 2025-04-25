#pragma once

#include "math/Matrix4.h"
#include "math/OBB.h"

namespace Engine {
using namespace Math;

namespace Graphics {
// Frustum Class:
// Represents a viewing frustum (camera, light, etc.)
// represented by a frustum matrix.
// Can be used to perform frustum culling.
class Frustum {
  private:
    // A matrix that converts world-space coordinates to the
    // camera's frustum space (normalized unit cube) space.
    // By D3D convention, this is space x in [-1,1],
    // y in [-1,1], z in [0, 1]
    Matrix4 m_world_to_frustum;
    Matrix4 m_frustum_to_world;

    Vector3 camera_pos;

  public:
    Frustum(const Matrix4& m_world_to_frustum);

    Vector3 toWorldSpace(const Vector3& frustum_coords) const;
    Vector3 toFrustumSpace(const Vector3& world_space) const;

    // Fills a size 8+ array with the frustum points
    void fillArrWithFrustumPoints(Vector3* point_arr) const;
    void fillArrWithWorldPoints(Vector3* point_arr) const;

    bool intersectsOBB(const OBB& obb) const;

    // Return frustum to world coordinates
    const Matrix4& getFrustumToWorldMatrix() const;
};

} // namespace Graphics
} // namespace Engine