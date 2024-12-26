#include "Camera.h"

#include <math.h>

#include "math/Compute.h"

#define ASPECT_RATIO (1920.f / 1080.f)

namespace Engine {
namespace Graphics {

Camera::Camera() {
    fov = 1.2f;
    z_near = 1.f;
    z_far = 500.f;

    transform = new Transform();
}
Camera::~Camera() = default;

// GetTransform:
// Returns the camera's transform
const Transform* Camera::getTransform() const { return transform; }

Transform* Camera::getTransform() { return transform; }

// GetFov:
// Returns the camera's FOV
float Camera::getFOV() const { return fov; }

// GetZNear:
// Returns the distance the Z-Near plane is from the camera.
// Anything closer to the camera than this is clipped.
float Camera::getZNear() const { return z_near; }

// GetZFar:
// Returns the distance the Z-Far plane is from the camera.
// Anything further from the camera than this is clipped.
float Camera::getZFar() const { return z_far; }

// SetTransform:
// Sets the camera's transform to follow a particular transform.
// Can be used
void Camera::setTransform(Transform* _transform) { transform = _transform; }

// SetFOV:
// Set's the camera's FOV. Clamped to prevent excessively wide
// FOVs.
void Camera::setFOV(float new_fov) {
    fov = Compute::Clamp(new_fov, 0.5f, PI - 0.5f);
}

// SetZNear:
// Set the distance of the Z-Near plane.
void Camera::setZNear(float new_znear) { z_near = new_znear; }

// SetZFar:
// Set the distance of the Z-Far plane
void Camera::setZFar(float new_zfar) { z_far = new_zfar; }

// Camera -> World Matrix
const Matrix4 Camera::getWorldToCameraMatrix(void) const {
    return transform->transformMatrix().inverse();
}

// Camera -> Projected Space Matrix
const Matrix4 Camera::getProjectionMatrix(void) const {
    Matrix4 projection_matrix = Matrix4();
    float fov_factor = cosf(fov / 2.f) / sinf(fov / 2.f);

    projection_matrix[0][0] = fov_factor / ASPECT_RATIO;
    projection_matrix[1][1] = fov_factor;
    projection_matrix[2][2] = z_far / (z_far - z_near);
    projection_matrix[2][3] = 1;
    projection_matrix[3][2] = (z_near * z_far) / (z_near - z_far);

    return projection_matrix;
}
} // namespace Graphics
} // namespace Engine