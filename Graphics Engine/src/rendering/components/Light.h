#pragma once

#include "Camera.h"

#include "rendering/Direct3D11.h"
#include "rendering/Shader.h"

#include "math/Color.h"
#include "math/Matrix4.h"

namespace Engine {
namespace Graphics {

enum ShadowMapQuality {
    QUALITY_0 = 64, 
    QUALITY_1 = 128,
    QUALITY_2 = 256,
    QUALITY_3 = 512
};

// LightComponent Class:
// Represents a directional light. Lights create shadows, and we create them
// using a shadow mapping technique.
// The "direction" of the light's view is given by the direction of its rotated
// +Z axis. To rotate a light, simply rotate its transform.
class Light : public Camera {
  protected:
    // Light emission color
    Color color;

    // Shadow map texture, and associated data to
    // let us bind it to the shader and render to it.
    ID3D11Texture2D* shadow_map;

    // Enables rendering to the texture
    D3D11_VIEWPORT viewport;
    ID3D11DepthStencilView* depth_stencil_view;

    // Enables use of / sampling of the texture in shaders
    ID3D11ShaderResourceView* shader_resource_view;
    ID3D11SamplerState* sampler_state;

  public:
    Light(ID3D11Device* device, ShadowMapQuality quality);
    ~Light();

    // Accessors of the Light's Data
    Color& getColor();

    D3D11_VIEWPORT& getViewport();
    ID3D11DepthStencilView*& getDepthView();

    ID3D11ShaderResourceView*& getShaderView();
    ID3D11SamplerState*& getSampler();
};
} // namespace Graphics
} // namespace Engine