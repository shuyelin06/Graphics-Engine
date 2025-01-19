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

// GetWorldToLightMatrix:
// Generates and returns the light space matrix
const Matrix4 ShadowLight::getWorldToLightMatrix(void) const {
    return transform->transformMatrix().inverse();
}
// GetProjectionMatrix:
// Generates and returns the projection matrix
const Matrix4 ShadowLight::getProjectionMatrix(void) const {
    Matrix4 projection_matrix = Matrix4();

    const float left = -20.f;
    const float right = 20.f;
    const float bottom = -20.f;
    const float top = 20.f;
    const float z_near = 5.f;
    const float z_far = 400.f;

    projection_matrix[0][0] = 2 / (right - left);
    projection_matrix[1][1] = 2 / (top - bottom);
    projection_matrix[2][2] = 1 / (z_far - z_near);
    projection_matrix[3][3] = 1;

    projection_matrix[3][0] = -(right + left) / (right - left);
    projection_matrix[3][1] = -(top + bottom) / (top - bottom);
    projection_matrix[3][2] = -(z_near) / (z_far - z_near);

    return projection_matrix;
}

} // namespace Graphics
} // namespace Engine