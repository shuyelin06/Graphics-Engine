#pragma once

#include <string>
#include <vector>

#include "FileReader.h"
#include "TextureBuilder.h"

namespace Engine {
namespace Graphics {
// PNGFile Class:
// Class that provides an interface for reading and writing
// PNG files. Internally uses the lodepng library to do this.
class PNGFile {
  private:
    std::string path;

  public:
    PNGFile(const std::string& file_path);

    bool writePNGData(ID3D11Device* device, ID3D11DeviceContext* context,
                      ID3D11Texture2D* texture);

    static void ReadPNGData(const std::vector<uint8_t>& data,
                            TextureBuilder& builder);
};

} // namespace Graphics
} // namespace Engine