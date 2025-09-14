#pragma once

#include "datamodel/DMBinding.h"
#include "datamodel/objects/DMCamera.h"

#include "math/Matrix4.h"
#include "math/OBB.h"
#include "math/Transform.h"

#include "Frustum.h"

namespace Engine {
using namespace Math;
using namespace Datamodel;

namespace Graphics {
// Camera Class:
// Represents the scene's camera, where everything
// on the screen is rendered from the camera's point of view.
// Unless otherwise rotated, the camera's default view
// is in the +Z axis.
class Camera : public DMBinding {
  protected:
    float fov;
    float z_near, z_far;

    // Frustum Matrix
    // Projects camera space coordiantes into the normalized
    // unit cube from [-1,1] x [-1,1] x [0,1]
    Matrix4 frustum_matrix;

    // Transform
    // Mirrors the camera object's transform,
    // and is used to compute the local to world matrix
    Transform transform;

    void pullDatamodelDataImpl(Object* obj) override;

  public:
    Camera(Object* dm_camera);
    ~Camera();

    // Get the camera's attributes
    const Transform& getTransform() const;
    const Vector3& getPosition() const;

    float getZNear() const;
    float getZFar() const;

    Frustum frustum() const;

    // World -> Camera Matrix
    const Matrix4 getWorldToCameraMatrix(void) const;
    // Camera -> Projected Space Matrix
    const Matrix4 getFrustumMatrix(void) const;

  private:
    void setFrustumMatrix(float fov, float z_near, float z_far);
};
} // namespace Graphics
} // namespace Engine