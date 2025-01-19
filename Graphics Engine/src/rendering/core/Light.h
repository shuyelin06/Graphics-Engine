#pragma once

#include "Camera.h"

#include "rendering/Direct3D11.h"
#include "rendering/Shader.h"
#include "rendering/util/TextureAtlas.h"

#include "math/Color.h"
#include "math/Matrix4.h"

namespace Engine {
namespace Graphics {

enum ShadowMapQuality {
    QUALITY_0 = 64,
    QUALITY_1 = 128,
    QUALITY_2 = 256,
    QUALITY_3 = 512,
    QUALITY_DEFAULT = QUALITY_1
};

struct AtlasTransform {
    float x, y, width, height;
};

// LightComponent Class:
// Represents a directional light. Lights create shadows, and we create them
// using a shadow mapping technique.
// The "direction" of the light's view is given by the direction of its rotated
// +Z axis. To rotate a light, simply rotate its transform.
class Light : public Camera {
  public:
    // TEMP
    static TextureAtlas* shadow_atlas;

  protected:
    // Light emission color
    Color color;

    // Viewport that must be used to write into the part
    // of the texture atlas allocated for the light
    D3D11_VIEWPORT viewport;
    AtlasTransform atlas_transform;

  public:
    Light(ID3D11Device* device, ShadowMapQuality quality);
    ~Light();

    // Accessors of the Light's Data
    Color& getColor();
    D3D11_VIEWPORT& getViewport();
    AtlasTransform& getAtlasTransform();

    const Matrix4 getProjectionMatrix(void) const;
};
} // namespace Graphics
} // namespace Engine