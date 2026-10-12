#include "PNGFile.h"

#include <assert.h>
#include <vector>

// The PNGFile uses the lodepng library to read PNG files.
// See https://github.com/lvandeve/lodepng
#include "lodepng/lodepng.h"

#define SUCCESS 1
#define FAILURE 0

namespace Engine
{
namespace Graphics
{
bool PNGFile::ReadPNGData(DataType type,
                          const std::vector<uint8_t>& data,
                          TextureBuilder& builder)
{
    // Run lodepng to decode my png file
    std::vector<uint8_t> image;
    unsigned int width, height;
    unsigned int error = lodepng::decode(image, width, height, data);
    if (error)
        return false;

    // Parse content of image into a format the engine can use. lodepng
    // automatically converts the PNG into RGBA values.
    switch (type)
    {
    case DataType::kAlbedo:
        builder.reset(width, height, TextureLayout::R8G8B8A8_UNORM);
        break;
    case DataType::kDisplacement:
        builder.reset(width, height, TextureLayout::R32_FLOAT);
        break;
    default:
        assert(false && "Unsupported data type");
        return false;
        break;
    }

    TextureColor texelOut;
    uint8_t pixelData[4] = {};

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            const unsigned int index = (y * width + x) * 4;
            memcpy(&pixelData, &image[index], 4 * sizeof(uint8_t));

            if (type == DataType::kAlbedo)
            {
                TextureColor::UNormR8G8B8A8& rgba =
                    texelOut.asType<TextureColor::UNormR8G8B8A8>();
                memcpy(&rgba.r, pixelData, 4 * sizeof(uint8_t));
            } // TODO: This does not preserve the original data well as we are forcing decoding into 8 bits.
            else if (type == DataType::kDisplacement)
            {
                TextureColor::FloatR32& f =
                    texelOut.asType<TextureColor::FloatR32>();
                f.r = pixelData[0] / 255.f;
                assert(f.r > 0.f);
            }

            builder.setColor(x, y, texelOut);
        }
    }

    return true;
}

} // namespace Graphics
} // namespace Engine