#pragma once

#include "Camera.h"
#include "TextureAtlas.h"

#include "rendering/Direct3D11.h"
#include "rendering/Shader.h"

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
};

// Light Class:
// Represents a directional light. Lights create shadows, and we create them
// using a shadow mapping technique.
// The "direction" of the light's view is given by the direction of its rotated
// +Z axis. To rotate a light, simply rotate its transform.
class ShadowLight {
  private:
    friend class LightManager;

    Color color;
    ShadowMapViewport shadow_viewport;
    Matrix4 m_projection;

    ShadowLight(const ShadowMapViewport& view_port);
    ~ShadowLight();

  public:
    const Color& getColor() const;
    const ShadowMapViewport& getShadowmapViewport() const;

    void setOrthogonalMatrix(float size_y, float aspect_ratio, float z_near, float z_far);
    void setPerspectiveMatrix(float fov_y, float aspect_ratio, float z_near, float z_far);

    const Matrix4& getProjectionMatrix(void) const;
};
} // namespace Graphics
} // namespace Engine