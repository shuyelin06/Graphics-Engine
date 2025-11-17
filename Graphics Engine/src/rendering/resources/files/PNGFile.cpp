#include "PNGFile.h"

#include <assert.h>
#include <vector>

// The PNGFile uses the lodepng library to read PNG files.
// See https://github.com/lvandeve/lodepng
#include "lodepng/lodepng.h"

#include "rendering/Direct3D11.h"

#define SUCCESS 1
#define FAILURE 0

namespace Engine {
namespace Graphics {
PNGFile::PNGFile(const std::string& file_path) { path = file_path; }

// --- PNG File Writing ---
// Given a ID3D11Texture2D, writes its contents to a png file for exporting /
// reading externally.
bool PNGFile::writePNGData(ID3D11Device* device, ID3D11DeviceContext* context,
                           ID3D11Texture2D* texture) {
    // Get description of the texture
    D3D11_TEXTURE2D_DESC tex_desc;
    texture->GetDesc(&tex_desc);

    // We will copy the contents of this texture to a "staging texture",
    // which we can actually use to read the data from on the CPU side.
    // To create it, we will copy the current texture description and modify the
    // usage and binding
    D3D11_TEXTURE2D_DESC staging_desc = tex_desc;
    staging_desc.Usage = D3D11_USAGE_STAGING; // Allows copying from GPU -> CPU
    staging_desc.BindFlags = 0; // Will not be bound to any pipeline stage
    staging_desc.MiscFlags = 0;
    staging_desc.CPUAccessFlags =
        D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE; // CPU access flags

    // TODO: For now, this will only work for DXGI_FORMAT_R8G8B8A8_UNORM.
    // We can extend this to work for more textures
    assert(staging_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM);

    ID3D11Texture2D* staging_tex; // Create an empty staging texture
    device->CreateTexture2D(&staging_desc, nullptr, &staging_tex);
    assert(staging_tex != NULL);

    // We now copy our current texture to the staging texture
    context->CopyResource(staging_tex, texture);

    // Our resource is copied! We now map the resource so that we can access its
    // contents on the CPU side.
    D3D11_MAPPED_SUBRESOURCE mapped;
    context->Map(staging_tex, 0, D3D11_MAP_READ, 0, &mapped);

    // Extract my data
    const int width = mapped.RowPitch / 4;
    const int height = mapped.DepthPitch / mapped.RowPitch;

    uint8_t* pixel_data = static_cast<uint8_t*>(mapped.pData);

    // Use lodepng to write the png file
    std::vector<unsigned char> image_data;

    for (int i = 0; i < width * height * 4; i++)
        image_data.push_back(pixel_data[i]);

    unsigned int error =
        lodepng::encode(path.c_str(), image_data, width, height);

    // We unmap our resource and release it to free the memory.
    context->Unmap(staging_tex, 0);
    staging_tex->Release();

    if (error)
        return false;
    else
        return true;
}

void PNGFile::ReadPNGData(const std::vector<uint8_t>& data,
                          TextureBuilder& builder) {
    std::vector<uint8_t> image;
    unsigned int width, height;

    // Run lodepng to decode my png file
    unsigned int error = lodepng::decode(image, width, height, data);
    assert(!error);

    // Parse content of image into a format the engine can use. lodepng
    // automatically converts the PNG into RGBA values.
    builder.reset(width, height);
    
    TextureColor color;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const unsigned int index = (y * width + x) * 4;
            memcpy(&color, &image[index], 4 * sizeof(uint8_t));
            builder.setColor(x, y, color);
        }
    }
}

} // namespace Graphics
} // namespace Engine