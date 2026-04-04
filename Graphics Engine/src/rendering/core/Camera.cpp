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
Camera::Camera() = default;
Camera::~Camera() = default;

void Camera::update(const UpdatePacket& packet) {
    using CameraProperty = UpdatePacket::Property;

    switch (packet.type) {
    case CameraProperty::LocalMatrix: {
        const Matrix4& matrix = std::get<Matrix4>(packet.data);
        local_to_world_matrix = matrix;
    } break;

    case CameraProperty::FOV: {
        fov = std::get<float>(packet.data);
        computeFrustumMatrix();
    } break;

    case CameraProperty::ZNear: {
        z_near = std::get<float>(packet.data);
        computeFrustumMatrix();
    } break;

    case CameraProperty::ZFar: {
        z_far = std::get<float>(packet.data);
        computeFrustumMatrix();
    } break;
    }
}

void Camera::computeFrustumMatrix() {
    Matrix4 projection_matrix = Matrix4();

    const float fov_factor = cosf(fov / 2.f) / sinf(fov / 2.f);

    projection_matrix[0][0] = fov_factor / ASPECT_RATIO;
    projection_matrix[1][1] = fov_factor;
    projection_matrix[2][2] = z_far / (z_far - z_near);
    projection_matrix[2][3] = 1;
    projection_matrix[3][2] = (z_near * z_far) / (z_near - z_far);

    frustum_matrix = projection_matrix;
}

// --- Accessors ---
float Camera::getZNear() const { return z_near; }
float Camera::getZFar() const { return z_far; }
const Vector3 Camera::forward() const {
    Vector4 direc = local_to_world_matrix * Vector4(0, 0, 1, 1);
    return direc.xyz();
}
const Vector3& Camera::getPosition() const {
    Vector4 pos = local_to_world_matrix * Vector4(0, 0, 0, 1);
    return pos.xyz();
}

// GetFrustum:
// Returns an object which can be used to query the camera frustum.
Frustum Camera::frustum() const {
    const Matrix4 m_world_to_frustum =
        frustum_matrix * local_to_world_matrix.inverse();
    return Frustum(m_world_to_frustum);
}

// Camera -> World Matrix
const Matrix4 Camera::getWorldToCameraMatrix(void) const {
    return local_to_world_matrix.inverse();
}

// Camera -> Projected Space Matrix
const Matrix4 Camera::getFrustumMatrix(void) const { return frustum_matrix; }
} // namespace Graphics
} // namespace Engine