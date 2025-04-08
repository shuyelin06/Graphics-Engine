#include "TextureBuilder.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
TextureBuilder::TextureBuilder(UINT _width, UINT _height,
                               ID3D11Device* _device) {
    device = _device;

    texture_resource = new Texture(_width, _height);

    data.resize(_width * _height);
    clear({90, 34, 139, 255});
}

TextureBuilder::~TextureBuilder() = default;

// GenerateTexture:
// Generates a texture resource (for use in the rendering pipeline)
// given the data stored within the builder.
Texture* TextureBuilder::generate() {
    // Generate my GPU texture resource
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = texture_resource->width;
    tex_desc.Height = texture_resource->height;
    tex_desc.MipLevels = tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sr_data = {};
    sr_data.pSysMem = data.data();
    sr_data.SysMemPitch = texture_resource->width * 4; // Bytes per row
    sr_data.SysMemSlicePitch = texture_resource->width *
                               texture_resource->height * 4; // Total byte size

    device->CreateTexture2D(&tex_desc, &sr_data, &(texture_resource->texture));
    assert(texture_resource->texture != NULL);

    // Generate a shader view for my texture
    D3D11_SHADER_RESOURCE_VIEW_DESC tex_view;
    tex_view.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    tex_view.Texture2D.MostDetailedMip = 0;
    tex_view.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(texture_resource->texture, &tex_view,
                                     &(texture_resource->shader_view));

    Texture* output = texture_resource;
    texture_resource = nullptr;

    return output;
}

// SetColor:
// Sets a pixel of the texture to some color value
void TextureBuilder::setColor(UINT x, UINT y, const TextureColor& rgba) {
    assert(0 <= x && x < texture_resource->width);
    assert(0 <= y && y < texture_resource->height);

    data[y * texture_resource->width + x] = rgba;
}

// Clear:
// Clears the texture, setting all of the RGBA pixels to a particular color.
void TextureBuilder::clear(const TextureColor& rgba) {
    const UINT pixel_width = texture_resource->width;
    const UINT pixel_height = texture_resource->height;

    for (int i = 0; i < pixel_width * pixel_height; i++)
        data[i] = rgba;
}

// Reset:
// Resets the builder
void TextureBuilder::reset(unsigned int _width, unsigned int _height) {
    if (texture_resource != nullptr)
        delete texture_resource;
    texture_resource = new Texture(_width, _height);

    data.resize(_width * _height);
    clear({90, 34, 139, 255});
}

// --- Atlas Builder ---
AtlasBuilder::AtlasBuilder(UINT atlas_width, UINT atlas_height,
                           ID3D11Device* device)
    : TextureBuilder(atlas_width, atlas_height, device) {
    // Initialize my texture atlas
    atlas = new TextureAtlas(texture_resource);

    cur_region = nullptr;
}
AtlasBuilder::~AtlasBuilder() = default;

// AllocateRegion:
// Allocates a new region in the atlas for a new texture to be written to
const AtlasAllocation& AtlasBuilder::allocateRegion(UINT tex_width,
                                                    UINT tex_height) {
    const UINT allocation_id = atlas->allocateTexture(tex_width, tex_height);
    const AtlasAllocation& allocation = atlas->getAllocation(allocation_id);

    cur_region = &allocation;

    return allocation;
}

// Generates the texture for the atlas and returns the atlas.
TextureAtlas* AtlasBuilder::generate() {
    TextureBuilder::generate();

    TextureAtlas* output = atlas;
    atlas = nullptr;

    return output;
}

// Accessors:
// Access properties of the atlas
UINT AtlasBuilder::getAtlasWidth() const { return texture_resource->width; }
UINT AtlasBuilder::getAtlasHeight() const { return texture_resource->height; }

// SetColor:
// Sets the color of a pixel relative to the current region
void AtlasBuilder::setColor(UINT x, UINT y, const TextureColor& rgba) {
    // AllocateRegion should have been called before this
    assert(cur_region != nullptr);
    assert(x < cur_region->width);
    assert(y < cur_region->height);

    const UINT pixel_x = cur_region->x + x;
    const UINT pixel_y = cur_region->y + y;

    TextureBuilder::setColor(pixel_x, pixel_y, rgba);
}

// Clear:
// Clears the allocation region with an RGBA color
void AtlasBuilder::clear(const TextureColor& rgba) {
    // AllocateRegion should have been called before this
    assert(cur_region != nullptr);

    const UINT width = cur_region->width;
    const UINT height = cur_region->height;

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            setColor(x, y, rgba);
        }
    }
}

} // namespace Graphics
} // namespace Engine