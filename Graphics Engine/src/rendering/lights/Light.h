#pragma once

#include "../Direct3D11.h"
#include "../core/Frustum.h"
#include "../core/TextureAtlas.h"

#include "math/Color.h"
#include "math/Matrix4.h"
#include "math/Transform.h"

namespace Engine {
namespace Graphics {

// Struct ShadowMapViewport
// Represents where in the shadowmap_atlas the light's
// shadow_map is. Stored in pixel coordinates.
struct ShadowMapViewport {
    float x, y, width, height;

    D3D11_VIEWPORT toD3D11() const;
};

// Light Class:
// Represents a directional light. Lights create shadows, and we generate these
// shadows shadow mapping. The "direction" of the light's view is given by the
// direction of its rotated +Z axis. Lights have a position and rotation in
// space that defines what regions they light up.
class ShadowLight {
  private:
    Color color;
    ShadowMapViewport shadow_viewport;

    Matrix4 m_world;      // Local to World
    Matrix4 m_projection; // World to Projection

  public:
    ShadowLight(const ShadowMapViewport& view_port);
    ~ShadowLight();

    // Accessors
    const Color& getColor() const;
    const ShadowMapViewport& getShadowmapViewport() const;

    const Matrix4& getWorldMatrix(void) const;
    const Matrix4& getFrustumMatrix(void) const;

    Vector3 getPosition(void) const;
    Frustum frustum() const;

    // Setters
    void setPosition(const Vector3& position);
    void setRotation(const Quaternion& rotation);
    void setWorldMatrix(const Matrix4& matrix);

    void setColor(const Color& color);

    void setOrthogonalFrustum(float size_y, float aspect_ratio, float z_near,
                              float z_far);
    void setPerspectiveFrustum(float fov_y, float aspect_ratio, float z_near,
                               float z_far);
};
} // namespace Graphics
} // namespace Engine