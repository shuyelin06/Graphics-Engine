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
CameraComponent::CameraComponent(Object* object) : Component(object) {
    setFrustumMatrix(1.2f, 5.f, 500.f);
}
CameraComponent::~CameraComponent() = default;

// --- Update ---
void CameraComponent::update() { transform = object->getTransform(); }

// --- Accessors ---
float CameraComponent::getZNear() const { return z_near; }
float CameraComponent::getZFar() const { return z_far; }
const Transform& CameraComponent::getTransform() const { return transform; }
const Vector3& CameraComponent::getPosition() const {
    return transform.getPosition();
}

// GetFrustum:
// Returns an object which can be used to query the camera frustum.
Frustum CameraComponent::frustum() const {
    const Matrix4 m_world_to_frustum =
        frustum_matrix * object->getTransform().transformMatrix().inverse();
    return Frustum(m_world_to_frustum);
}

// SetFrustuMatrix:
// Updates the camera frustum (projection) matrix
void CameraComponent::setFrustumMatrix(float fov, float _z_near, float _z_far) {
    z_near = _z_near;
    z_far = _z_far;

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
const Matrix4 CameraComponent::getWorldToCameraMatrix(void) const {
    return transform.transformMatrix().inverse();
}

// Camera -> Projected Space Matrix
const Matrix4 CameraComponent::getFrustumMatrix(void) const {
    return frustum_matrix;
}
} // namespace Graphics
} // namespace Engine