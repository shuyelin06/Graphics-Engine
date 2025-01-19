#include "Light.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
TextureAtlas* Light::shadow_atlas = nullptr;

// Constructor:
// Initializes a texture resource for use in the shadow mapping. The
// device is needed to intialize
Light::Light(ID3D11Device* device, ShadowMapQuality quality) : Camera() {
    color = Color(1.0f, 1.0f, 1.0f);

    // Create my viewport, so that on binding, we will write to the light's
    // depth texture.
    const UINT alloc_index = shadow_atlas->allocateTexture(quality, quality);
    const AtlasAllocation& allocation = shadow_atlas->getAllocation(alloc_index);
    const Texture* texture = shadow_atlas->getTexture();

    viewport = {};
    viewport.Width = allocation.width;
    viewport.Height = allocation.height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = allocation.x;
    viewport.TopLeftY = allocation.y;

    atlas_transform = {};
    atlas_transform.x = float(allocation.x) / float(texture->width);
    atlas_transform.y = float(allocation.y) / float(allocation.height);
    atlas_transform.width = float(allocation.width) / float(texture->width);
    atlas_transform.height = float(allocation.height) / float(texture->height);
}

Light::~Light() = default;

Color& Light::getColor() { return color; }

D3D11_VIEWPORT& Light::getViewport() { return viewport; }
AtlasTransform& Light::getAtlasTransform() { return atlas_transform; }

const Matrix4 Light::getProjectionMatrix(void) const {
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

    projection_matrix[3][0] = - (right + left) / (right - left);
    projection_matrix[3][1] = - (top + bottom) / (top - bottom);
    projection_matrix[3][2] = - (z_near) / (z_far - z_near);

    return projection_matrix;
}

} // namespace Graphics
} // namespace Engine