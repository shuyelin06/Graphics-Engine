#pragma once

#include <string>

#include "../Direct3D11.h"
#include "../core/TextureBuilder.h"

namespace Engine
{
namespace Graphics
{
// Stores PNG Chunk data prior to being processed
struct PNGChunk {
    uint8_t chunk_type[4];
    std::vector<unsigned char> chunk_data;
};

// PNGFile Class:
// Class that provides an interface for working with 
// PNG files. Allows reading to and writing from these files.
class PNGFile
{
private:
    std::string path;

public:
    PNGFile(const std::string& file_path);
    
    bool writeTextureToFile(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* texture);
    bool readTextureFromFile(TextureBuilder& builder);

private:
    void flip(void* buffer, size_t n_bytes);
    uint32_t checksum32(uint8_t* data, size_t size);

    uint8_t readPNGChunk(FILE* file, PNGChunk& chunk);
    void writePNGChunk(FILE* file, uint8_t type[4], uint8_t* data,
                       uint32_t data_size);
};

}}