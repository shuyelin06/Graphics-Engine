#include "Light.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
// Constructor:
// Initializes a texture resource for use in the shadow mapping. The
// device is needed to intialize
ShadowLight::ShadowLight(const ShadowMapViewport& view_port) {
    transform = new Transform();

    color = Color(1.0f, 1.0f, 1.0f);
    shadow_viewport = view_port;

    setOrthogonalMatrix(40.f, 1.0f, 5.0f, 200.f);
}

ShadowLight::~ShadowLight() = default;

// Getters:
// Return various properties of the light
const Color& ShadowLight::getColor() const { return color; }
const ShadowMapViewport& ShadowLight::getShadowmapViewport() const {
    return shadow_viewport;
}

// GetTransform:
// Returns a pointer to the transform that allows modification
Transform* ShadowLight::getTransform() { return transform; }

// SetProjectionMatrix:
// Sets the light's projection matrix to be orthogonal or perspective,
// with given parameters
void ShadowLight::setOrthogonalMatrix(float size_y, float aspect_ratio,
                                      float z_near, float z_far) {
    m_projection = Matrix4();

    const float size_x = size_y * aspect_ratio;

    m_projection[0][0] = 2 / size_x;
    m_projection[1][1] = 2 / size_y;
    m_projection[2][2] = 1 / (z_far - z_near);
    m_projection[3][3] = 1;
    m_projection[3][2] = -(z_near) / (z_far - z_near);
}
void ShadowLight::setPerspectiveMatrix(float fov_y, float aspect_ratio,
                                       float z_near, float z_far) {
    m_projection = Matrix4();

    const float fov_factor = cosf(fov_y / 2.f) / sinf(fov_y / 2.f);

    m_projection[0][0] = fov_factor / aspect_ratio;
    m_projection[1][1] = fov_factor;
    m_projection[2][2] = z_far / (z_far - z_near);
    m_projection[2][3] = 1;
    m_projection[3][2] = (z_near * z_far) / (z_near - z_far);
}

// GetWorldToLightMatrix:
// Generates and returns the light space matrix
const Matrix4 ShadowLight::getWorldToLightMatrix(void) const {
    return transform->transformMatrix().inverse();
}
// GetProjectionMatrix:
// Generates and returns the projection matrix
const Matrix4& ShadowLight::getProjectionMatrix(void) const {
    return m_projection;
}

} // namespace Graphics
} // namespace Engine