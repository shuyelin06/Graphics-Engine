#pragma once

#include "math/Matrix4.h"
#include "math/OBB.h"
#include "math/Transform.h"

#include "Frustum.h"

namespace Engine {
using namespace Math;

namespace Graphics {
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

    Frustum frustum() const;

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