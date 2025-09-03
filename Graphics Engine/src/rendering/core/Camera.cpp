#include "Camera.h"

#include <algorithm>
#include <float.h>
#include <math.h>

#include "datamodel/Object.h"
#include "math/Compute.h"

#define ASPECT_RATIO (1920.f / 1080.f)

namespace Engine {
namespace Graphics {
// --- Camera ---
Camera::Camera() { setFrustumMatrix(1.2f, 5.f, 500.f); }
Camera::~Camera() = default;

// -- Pull Datamodel Data ---
void Camera::pullDatamodelData(DMCamera* dm_camera) {
    transform = dm_camera->getTransform();
    setFrustumMatrix(dm_camera->getFOV(), dm_camera->getZNear(),
                     dm_camera->getZFar());
}

// --- Accessors ---
float Camera::getZNear() const { return z_near; }
float Camera::getZFar() const { return z_far; }
const Transform& Camera::getTransform() const { return transform; }
const Vector3& Camera::getPosition() const { return transform.getPosition(); }

// GetFrustum:
// Returns an object which can be used to query the camera frustum.
Frustum Camera::frustum() const {
    const Matrix4 m_world_to_frustum =
        frustum_matrix * transform.transformMatrix().inverse();
    return Frustum(m_world_to_frustum);
}

// SetFrustuMatrix:
// Updates the camera frustum (projection) matrix
void Camera::setFrustumMatrix(float _fov, float _z_near, float _z_far) {
    z_near = _z_near;
    z_far = _z_far;
    fov = _fov;

    Matrix4 projection_matrix = Matrix4();

    const float fov_factor = cosf(fov / 2.f) / sinf(fov / 2.f);

    projection_matrix[0][0] = fov_factor / ASPECT_RATIO;
    projection_matrix[1][1] = fov_factor;
    projection_matrix[2][2] = z_far / (z_far - z_near);
    projection_matrix[2][3] = 1;
    projection_matrix[3][2] = (z_near * z_far) / (z_near - z_far);

    frustum_matrix = projection_matrix;
}

// Camera -> World Matrix
const Matrix4 Camera::getWorldToCameraMatrix(void) const {
    return transform.transformMatrix().inverse();
}

// Camera -> Projected Space Matrix
const Matrix4 Camera::getFrustumMatrix(void) const { return frustum_matrix; }
} // namespace Graphics
} // namespace Engine