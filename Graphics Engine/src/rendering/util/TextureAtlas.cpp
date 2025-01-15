#include "TextureAtlas.h"

#include <assert.h>

#include "rendering/core/TextureBuilder.h"

namespace Engine {
namespace Graphics {
TextureAtlas::TextureAtlas(UINT _width, UINT _height) : textures() {
    texture = nullptr;

    width = _width;
    height = _height;

    x_alloc_bound = 0;
    y_alloc_bound = 0;
}
TextureAtlas::~TextureAtlas() = default;

// Initialize:
// Generates the GPU texture resource
void TextureAtlas::initialize(ID3D11Device* device, DXGI_FORMAT format,
                              UINT bind_flags) {
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = tex_desc.ArraySize = 1;
    tex_desc.Format = format;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = bind_flags;
    tex_desc.CPUAccessFlags = 0;

    device->CreateTexture2D(&tex_desc, NULL, &texture);
    assert(texture != nullptr);
}

// GetAllocation:
// Returns the allocation for a texture 
const TextureAllocation& TextureAtlas::getAllocation(UINT index) const
{
    return textures[index];
}

// GetAtlasCoordinates:
// Transform the texture coordinates into atlas coordinates.
// Ignores texture addressing: TODO
Vector2 TextureAtlas::getAtlasCoordinates(UINT texture, Vector2 tex_coords)
{
    const TextureAllocation& allocation = textures[texture];

    const float x = allocation.x + tex_coords.u * allocation.width;
    const float y = allocation.y + tex_coords.v * allocation.height;

    return Vector2(x,y);
}

// AllocateTexture:
// Allocates space for a texture of specified width and height in the atlas.
// Uses a 2D Rectangle Packing Algorithm to do this tightly.
// This is a open area of research, and the algorithm could be iterated on.
// Returns the index of the allocation in the atlas, as a unique ID to the allocation.
UINT TextureAtlas::allocateTexture(UINT tex_width, UINT tex_height)
{
    UINT index;
    bool allocated = false;

    // Naively pack left -> right
    if (x_alloc_bound + tex_width <= width) {
        TextureAllocation allocation;
        allocation.x = x_alloc_bound;
        allocation.y = 0;
        allocation.width = tex_width;
        allocation.height = tex_height;

        x_alloc_bound += tex_width;

        index = textures.size();
        textures.push_back(allocation);
        
        allocated = true;
    }

    // Throw exception if allocation failed
    assert(allocated); 

    return index;
}

} // namespace Graphics
} // namespace Engine