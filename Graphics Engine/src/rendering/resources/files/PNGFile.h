#pragma once

#include <string>
#include <vector>

#include "../TextureBuilder.h"

namespace Engine
{
namespace Graphics
{
// PNGFile:
// Class that provides an interface for reading and writing
// PNG files. Internally uses the lodepng library to do this.
namespace PNGFile
{
enum class DataType : uint8_t
{
    kAlbedo = 0
};

bool ReadPNGData(DataType type,
                 const std::vector<uint8_t>& data,
                 TextureBuilder& builder);
}; // namespace PNGFile

} // namespace Graphics
} // namespace Engine