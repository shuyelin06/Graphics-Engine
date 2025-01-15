#include "TextureBuilder.h"

#include <assert.h>

namespace Engine {
namespace Graphics {

TextureBuilder::TextureBuilder(ID3D11Device* _device, UINT _width,
                               UINT _height) {
    device = _device;

    pixel_width = _width;
    pixel_height = _height;

    data.resize(pixel_width * pixel_height);
    clear({90, 34, 139, 255});
}

TextureBuilder::~TextureBuilder() = default;

// GenerateTexture:
// Generates a texture resource (for use in the rendering pipeline)
// given the data stored within the builder.
Texture* TextureBuilder::generate() {
    Texture* texture_resource = new Texture();
    texture_resource->width = pixel_width;
    texture_resource->height = pixel_height;

    // Generate my GPU texture resource
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = pixel_width;
    tex_desc.Height = pixel_height;
    tex_desc.MipLevels = tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sr_data = {};
    sr_data.pSysMem = data.data();
    sr_data.SysMemPitch = pixel_width * 4; // Bytes per row
    sr_data.SysMemSlicePitch =
        pixel_width * pixel_height * 4; // Total byte size

    device->CreateTexture2D(&tex_desc, &sr_data, &(texture_resource->texture));
    assert(texture_resource->texture != NULL);

    // Generate a shader view for my texture
    D3D11_SHADER_RESOURCE_VIEW_DESC tex_view;
    tex_view.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    tex_view.Texture2D.MostDetailedMip = 0;
    tex_view.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(texture_resource->texture, &tex_view,
                                     &(texture_resource->view));

    return texture_resource;
}

// SetColor:
// Sets a pixel of the texture to some color value
void TextureBuilder::setColor(UINT x, UINT y, const TextureColor& rgba) {
    assert(0 <= x && x < pixel_width);
    assert(0 <= y && y < pixel_height);

    data[y * pixel_width + x] = rgba;
}

// Clear:
// Clears the texture, setting all of the RGBA pixels to a particular color.
void TextureBuilder::clear(const TextureColor& rgba) {
    for (int i = 0; i < pixel_width * pixel_height; i++)
        data[i] = rgba;
}

// Reset:
// Resets the builder
void TextureBuilder::reset(unsigned int _width, unsigned int _height) {
    pixel_width = _width;
    pixel_height = _height;

    data.resize(pixel_width * pixel_height);
    clear({90, 34, 139, 255});
}

} // namespace Graphics
} // namespace Engine