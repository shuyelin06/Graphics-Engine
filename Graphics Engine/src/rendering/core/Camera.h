#pragma once

#include "math/Matrix4.h"
#include "math/Transform.h"

namespace Engine {
using namespace Math;

namespace Graphics {
class Camera;

// CameraFrustum Struct:
// Stores data about a camera frustum that can be queried and used.
// Used in calculations for the sun's shadow cascade.
class CameraFrustum {
  private:
    // A matrix that converts world-space coordinates to the
    // camera's frustum space (normalized unit cube) space.
    // By D3D convention, this is space x in [-1,1],
    // y in [-1,1], z in [0, 1]
    Matrix4 m_world_to_frustum;
    Matrix4 m_frustum_to_world;
    
    Vector3 camera_pos;

  public:
    CameraFrustum(const Camera& camera);

    Vector3 toWorldSpace(const Vector3& frustum_coords) const;
    Vector3 toFrustumSpace(const Vector3& world_space) const;
    Vector3 getCameraPosition() const;
};

// Camera Class:
// Represents the scene's camera, where everything
// on the screen is rendered from the camera's point of view.
// Unless otherwise rotated, the camera's default view
// is in the +Z axis.
class Camera {
  protected:
    // Field of view
    float fov;

    // Z-near and z-far viewing planes
    float z_near;
    float z_far;

    // Transform
    Transform* transform;

    // Frustum Matrix
    // Projects camera space coordiantes into the normalized 
    // unit cube from [-1,1] x [-1,1] x [0,1]
    Matrix4 frustum_matrix;

  public:
    Camera();
    ~Camera();

    // Get the camera's attributes
    const Transform* getTransform() const;
    Transform* getTransform();

    CameraFrustum getFrustum() const;

    // Set the camera's attributes
    void setTransform(Transform* transform);
    void setFrustumMatrix(float fov, float z_near, float z_far);

    // World -> Camera Matrix
    const Matrix4 getWorldToCameraMatrix(void) const;

    // Camera -> Projected Space Matrix
    const Matrix4 getFrustumMatrix(void) const;
};
} // namespace Graphics
} // namespace Engine