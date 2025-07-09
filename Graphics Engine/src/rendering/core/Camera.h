#pragma once

#include "datamodel/Component.h"

#include "math/Matrix4.h"
#include "math/OBB.h"
#include "math/Transform.h"

#include "Frustum.h"

namespace Engine {
using namespace Datamodel;
using namespace Math;

namespace Graphics {
// Camera Class:
// Represents the scene's camera, where everything
// on the screen is rendered from the camera's point of view.
// Unless otherwise rotated, the camera's default view
// is in the +Z axis.
class CameraComponent : public Component {
  protected:
    // Field of view
    float fov;

    // Z-near and z-far viewing planes
    float z_near, z_far;

    // Frustum Matrix
    // Projects camera space coordiantes into the normalized
    // unit cube from [-1,1] x [-1,1] x [0,1]
    Matrix4 frustum_matrix;

    // Transform
    // Mirrors the camera object's transform,
    // and is used to compute the local to world matrix
    Transform transform;

  public:
    CameraComponent(Object* object);
    ~CameraComponent();

    // OVERRIDE:
    // Pulls the object's transform
    void update();
    void imGuiConfig();

    // Get the camera's attributes
    float getZNear() const;
    float getZFar() const;

    const Transform& getTransform() const;
    const Vector3& getPosition() const;

    Frustum frustum() const;

    // Set the camera's attributes
    void setFrustumMatrix(float fov, float z_near, float z_far);

    // World -> Camera Matrix
    const Matrix4 getWorldToCameraMatrix(void) const;

    // Camera -> Projected Space Matrix
    const Matrix4 getFrustumMatrix(void) const;
};
} // namespace Graphics
} // namespace Engine