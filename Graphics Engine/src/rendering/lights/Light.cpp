#include "Light.h"

#include <assert.h>

#include "datamodel/Object.h"
#include "../pipeline/ConstantBuffer.h"

namespace Engine {
namespace Graphics {
// ToD3D11:
// Convert a ShadowMapViewport to the D3D11 structure
D3D11_VIEWPORT ShadowMapViewport::toD3D11() const {
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    return viewport;
}

// Constructor:
// Initializes a texture resource for use in the shadow mapping. The
// device is needed to intialize
ShadowLight::ShadowLight(Object* object, const ShadowMapViewport& view_port)
    : Component(object) {
    m_world = Matrix4::Identity();
    m_projection = Matrix4::Identity();

    color = Color(1.0f, 1.0f, 1.0f);
    shadow_viewport = view_port;

    setPerspectiveFrustum(1.f, 1.f, 5.0f, 60.f);
}

ShadowLight::~ShadowLight() = default;

// Datamodel Update:
void ShadowLight::update() {
    const Matrix4& m_world = object->getLocalMatrix();
    setWorldMatrix(m_world);
}

// Getters:
// Return various properties of the light
const Color& ShadowLight::getColor() const { return color; }
const ShadowMapViewport& ShadowLight::getShadowmapViewport() const {
    return shadow_viewport;
}

const Matrix4& ShadowLight::getWorldMatrix(void) const { return m_world; }
const Matrix4& ShadowLight::getFrustumMatrix(void) const {
    return m_projection;
}

Vector3 ShadowLight::getPosition(void) const { return m_world.column(3).xyz(); }

Frustum ShadowLight::frustum() const {
    const Matrix4 m_world_to_frustum =
        getFrustumMatrix() * getWorldMatrix().inverse();
    return Frustum(m_world_to_frustum);
}

// --- Setters ---
// SetPosition:
// Updates the last column of the local to world matrix to reflect the change
// in position
void ShadowLight::setPosition(const Vector3& position) {
    m_world.setColumn(3, Vector4(position, 1.f));
}

// SetRotation:
// Updates the first 3 columns of the local to world matrix to reflect the
// change in rotation
void ShadowLight::setRotation(const Quaternion& rotation) {
    const Matrix3 m_rotation = rotation.rotationMatrix3();
    m_world.setColumn(0, Vector4(m_rotation.column(0), 0.f));
    m_world.setColumn(1, Vector4(m_rotation.column(1), 0.f));
    m_world.setColumn(2, Vector4(m_rotation.column(2), 0.f));
}

// SetWorldMatrix:
// Sets the light's world matrix
void ShadowLight::setWorldMatrix(const Matrix4& matrix) { m_world = matrix; }

// SetColor:
// Updates the light's color/.
void ShadowLight::setColor(const Color& _color) { color = _color; }

// SetProjectionMatrix:
// Sets the light's projection matrix to be orthogonal or perspective,
// with given parameters
void ShadowLight::setOrthogonalFrustum(float size_y, float aspect_ratio,
                                       float z_near, float z_far) {
    m_projection = Matrix4();

    const float size_x = size_y * aspect_ratio;

    m_projection[0][0] = 2 / size_x;
    m_projection[1][1] = 2 / size_y;
    m_projection[2][2] = 1 / (z_far - z_near);
    m_projection[3][3] = 1;
    m_projection[3][2] = -(z_near) / (z_far - z_near);
}
void ShadowLight::setPerspectiveFrustum(float fov_y, float aspect_ratio,
                                        float z_near, float z_far) {
    m_projection = Matrix4();

    const float fov_factor = cosf(fov_y / 2.f) / sinf(fov_y / 2.f);

    m_projection[0][0] = fov_factor / aspect_ratio;
    m_projection[1][1] = fov_factor;
    m_projection[2][2] = z_far / (z_far - z_near);
    m_projection[2][3] = 1;
    m_projection[3][2] = (z_near * z_far) / (z_near - z_far);
}

} // namespace Graphics
} // namespace Engine