#include "Camera.h"

#include <math.h>

#include "math/Compute.h"

#define ASPECT_RATIO (1920.f / 1080.f)

namespace Engine {
namespace Graphics {
// --- Camera Frustum ---
CameraFrustum::CameraFrustum(const Camera& camera) {
    m_world_to_frustum =
        camera.getFrustumMatrix() * camera.getWorldToCameraMatrix();
    m_frustum_to_world = camera.getTransform()->transformMatrix() * camera.getFrustumMatrix().inverse();

    camera_pos = camera.getTransform()->getPosition();
}

Vector3 CameraFrustum::toWorldSpace(const Vector3& frustum_coords) const {
    Vector4 transformed = m_frustum_to_world * Vector4(frustum_coords, 1.f);
    transformed = transformed / transformed.w;
    return transformed.xyz();
}

Vector3 CameraFrustum::toFrustumSpace(const Vector3& world_space) const {
    Vector4 transformed = m_world_to_frustum * Vector4(world_space, 1.f);
    transformed = transformed / transformed.w;
    return transformed.xyz();
}

Vector3 CameraFrustum::getCameraPosition() const {
    return camera_pos;
}

// --- Camera ---
Camera::Camera() {
    setFrustumMatrix(1.2f, 5.f, 300.f);
    setTransform(new Transform());
}
Camera::~Camera() = default;

// GetTransform:
// Returns the camera's transform
const Transform* Camera::getTransform() const { return transform; }
Transform* Camera::getTransform() { return transform; }

// GetFrustum:
// Returns an object which can be used to query the camera frustum.
CameraFrustum Camera::getFrustum() const { return CameraFrustum(*this); }

// SetTransform:
// Sets the camera's transform to follow a particular transform.
// Can be used
void Camera::setTransform(Transform* _transform) { transform = _transform; }

// SetFrustuMatrix:
// Updates the camera frustum (projection) matrix
void Camera::setFrustumMatrix(float fov, float z_near, float z_far) {
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
    return transform->transformMatrix().inverse();
}

// Camera -> Projected Space Matrix
const Matrix4 Camera::getFrustumMatrix(void) const { return frustum_matrix; }
} // namespace Graphics
} // namespace Engine