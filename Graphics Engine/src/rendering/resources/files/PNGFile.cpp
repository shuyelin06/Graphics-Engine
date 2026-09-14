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
    default:
        assert(false && "Unsupported data type");
        return false;
        break;
    }

    TextureColor color;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            const unsigned int index = (y * width + x) * 4;
            memcpy(&color, &image[index], 4 * sizeof(uint8_t));
            builder.setColor(x, y, color);
        }
    }

    return true;
}

} // namespace Graphics
} // namespace Engine