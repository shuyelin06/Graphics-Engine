#include "TextureBuilder.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
TextureColor::TextureColor() = default;
TextureColor::TextureColor(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) {
    r = _r;
    g = _g;
    b = _b;
    a = _a;
}

TextureBuilder::TextureBuilder(UINT _width, UINT _height) {
    width = _width;
    height = _height;

    data.resize(width * height);
    clear({90, 34, 139, 255});
}

TextureBuilder::~TextureBuilder() = default;

// GenerateTexture:
// Generates a texture resource (for use in the rendering pipeline)
// given the data stored within the builder.
Texture* TextureBuilder::generate(ID3D11Device* device) {
    return generate(device, false);
}
Texture* TextureBuilder::generate(ID3D11Device* device, bool editable) {
    Texture* texture_resource = new Texture(width, height);

    // Generate my GPU texture resource
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    if (editable) {
        texture_resource->editable = true;
        tex_desc.Usage = D3D11_USAGE_DYNAMIC;
        tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    } else {
        tex_desc.Usage = D3D11_USAGE_DEFAULT;
        tex_desc.CPUAccessFlags = 0;
    }

    D3D11_SUBRESOURCE_DATA sr_data = {};
    sr_data.pSysMem = data.data();
    sr_data.SysMemPitch = width * 4;               // Bytes per row
    sr_data.SysMemSlicePitch = width * height * 4; // Total byte size

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

// Update:
// Given an editable texture (editable field must be true),
// uploads the builder's data to the texture.
// The dimensions MUST match.
void TextureBuilder::update(Texture* texture, ID3D11DeviceContext* context) {
    assert(texture->editable);
    assert(width == texture->width);
    assert(height == texture->height);

    // Write to my texture using Map / Unmap.
    D3D11_MAPPED_SUBRESOURCE sr;
    context->Map(texture->texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &sr);

    uint8_t* dest = reinterpret_cast<uint8_t*>(sr.pData);
    uint8_t* src = reinterpret_cast<uint8_t*>(data.data());
    for (UINT y = 0; y < height; ++y)
        memcpy(dest + y * sr.RowPitch, src + y * width * 4, width * 4);

    context->Unmap(texture->texture, 0);
}

// SetColor:
// Sets a pixel of the texture to some color value
void TextureBuilder::setColor(UINT x, UINT y, const TextureColor& rgba) {
    assert(0 <= x && x < width);
    assert(0 <= y && y < height);

    data[y * width + x] = rgba;
}

// Clear:
// Clears the texture, setting all of the RGBA pixels to a particular color.
void TextureBuilder::clear(const TextureColor& rgba) {
    const UINT pixel_width = width;
    const UINT pixel_height = height;

    for (int i = 0; i < pixel_width * pixel_height; i++)
        data[i] = rgba;
}

// Reset:
// Resets the builder
void TextureBuilder::reset(unsigned int _width, unsigned int _height) {
    data.resize(_width * _height);
    clear({90, 34, 139, 255});
}

// --- Atlas Builder ---
AtlasBuilder::AtlasBuilder(UINT atlas_width, UINT atlas_height)
    : TextureBuilder(atlas_width, atlas_height) {
    // Initialize my texture atlas
    atlas = new TextureAtlas(new Texture(atlas_width, atlas_height));

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
TextureAtlas* AtlasBuilder::generate(ID3D11Device* device) {
    Texture* tex = TextureBuilder::generate(device);
    atlas->setTexture(tex);

    TextureAtlas* output = atlas;
    atlas = nullptr;

    return output;
}

// Accessors:
// Access properties of the atlas
UINT AtlasBuilder::getAtlasWidth() const { return width; }
UINT AtlasBuilder::getAtlasHeight() const { return height; }

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