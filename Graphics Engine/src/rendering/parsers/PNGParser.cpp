#include "rendering/AssetManager.h"

#include <assert.h>
#include <vector>

#include <zlib.h>

#define SUCCESS 1
#define FAILURE 0

namespace Engine {
namespace Graphics {
// Stores PNG Chunk data prior to being processed
struct PNGChunk {
    uint8_t chunk_type[4];
    std::vector<unsigned char> chunk_data;
};

static uint8_t readPNGChunk(FILE* file, PNGChunk& chunk);
static void flip(void* buffer, size_t n_bytes); // Flips Endianness

// Simple PNG File Parser based from https://pyokagan.name/blog/2019-10-14-png/
// http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html
// Returns a dynamically allocated pointer to a texture on success,
// and NULL on fail.
bool AssetManager::LoadTextureFromPNG(TextureBuilder& builder, std::string path,
                                      std::string png_file) {
    // Open my PNG file, and return NULL if we failed to open the file
    const std::string file_name = path + png_file;

    FILE* file = fopen(file_name.c_str(), "rb");
    if (file == nullptr)
        return false;

    // First 8 bytes of the file must match the PNG magic number.
    {
        uint8_t png_code[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

        uint8_t read_data[8];
        fread(read_data, 1, 8, file);

        if (memcmp(read_data, png_code, 8) != 0)
            return false;
    }

    PNGChunk chunk;
    uint32_t width, height;
    uint8_t bit_depth, color_type, compression, filter, interlace;
    bool result;

    // The first chunk should be the 'IHDR' chunk, storing our image properties.
    {
        result = readPNGChunk(file, chunk);

        if (result != SUCCESS || memcmp(chunk.chunk_type, "IHDR", 4) != 0 ||
            chunk.chunk_data.size() != 13)
            return false;

        // Parse the fields of the IHDR chunk. They are (in order):
        // 1) Image (Pixel) Width: 4 Bytes
        memcpy(&width, &chunk.chunk_data[0], 4);
        flip(&width, sizeof(width));

        // 2) Image (Pixel) Height: 4 Bytes
        memcpy(&height, &chunk.chunk_data[4], 4);
        flip(&height, sizeof(height));

        // 3) Image Bit Depth (Bits / Sample): 1 Byte
        bit_depth = chunk.chunk_data[8];
        if (!(bit_depth == 1 || bit_depth == 2 || bit_depth == 4 ||
              bit_depth == 8 ||
              bit_depth == 16)) // Bit depth can only be 1,2,4,8,16
            return false;

        // 4) Color Type: 1 Byte
        color_type = chunk.chunk_data[9];

        // 5) Compression Method: 1 Byte
        compression = chunk.chunk_data[10];
        if (compression != 0) // Only compression == 0 supported
            return false;

        // 6) Filter Method: 1 Byte
        filter = chunk.chunk_data[11];
        if (filter != 0) // Only filter == 0 supported
            return false;

        // 7) Interlace Method
        interlace = chunk.chunk_data[12];
    }

    // Assumptions of this PNG spec.
    assert(interlace == 0);  // No interlacing
    assert(color_type == 6); // RGBA Channels
    assert(bit_depth == 8);  // 8 Bits per Sample

    // Prepare my builder to begin loading the texture.
    builder.reset(width, height);

    // We are dealing with a RGBA 8-bit image.
    std::vector<uint8_t> raw_data;

    while (readPNGChunk(file, chunk),
           memcmp(chunk.chunk_type, "IEND", 4) != 0) {
        if (memcmp(chunk.chunk_type, "IDAT", 4) == 0) {
            raw_data.insert(raw_data.end(), chunk.chunk_data.begin(),
                            chunk.chunk_data.end());
        }
    }

    // Decompress with zlib
    constexpr int BYTES_PER_PIXEL = 4;
    std::vector<uint8_t> decompressed;
    decompressed.resize(height * (1 + width * BYTES_PER_PIXEL));

    uLong decompressed_size = decompressed.size();
    uLong raw_size = raw_data.size();
    uncompress((Bytef*)&decompressed[0], &decompressed_size,
               (Bytef*)&raw_data[0], raw_size);

    // Read each scan-line, which has a width of (width * bytesPerPixel + 1),
    // where 1 accounts for the filtering method.
    for (int y = 0; y < height; y++) {
        uint8_t filter_method =
            decompressed[y * (width * BYTES_PER_PIXEL + 1) + 0];
        assert(filter_method == 0);

        for (int x = 0; x < width; x++) {
            uint8_t r = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                     (x * BYTES_PER_PIXEL) + 1];
            uint8_t g = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                     (x * BYTES_PER_PIXEL) + 2];
            uint8_t b = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                     (x * BYTES_PER_PIXEL) + 3];
            uint8_t a = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                     (x * BYTES_PER_PIXEL) + 4];

            builder.setColor(x, y, {r, g, b, a});
        }
    }

    return true;
}

// Reads a PNG Chunk. A PNG Chunk Consists of:
// 1) A 4-byte UINT giving the number of bytes in the data field
// 2) A 4-byte character sequence defining the chunk type
// 3) The bytes of data of the chunk
// 4) UNUSED: A 4-byte CRC calculated on fields (2) and (3), to check for data
// corruption. Returns 1 on success, 0 on failure.
uint8_t readPNGChunk(FILE* file, PNGChunk& chunk) {
    size_t bytes_read = 0;

    // Read the 4-byte length of the chunk.
    uint32_t length;

    bytes_read = fread(&length, 1, 4, file);
    if (bytes_read != 4) // Fail if 4 bytes were not read
        return FAILURE;

    flip(&length, sizeof(length));

    // Read the 4-byte chunk type. Each of them is 1-byte, so we don't need
    // to flip the endian-ness.
    bytes_read = fread(&(chunk.chunk_type), 1, 4, file);
    if (bytes_read != 4) // Fail if 4 bytes were not read
        return FAILURE;

    // If there is data to read, read the chunk's data.
    if (length > 0) {
        chunk.chunk_data.resize(length);
        bytes_read = fread(&(chunk.chunk_data[0]), 1, length, file);
        if (bytes_read != length)
            return FAILURE;
    }

    // Read the 4-byte Cyclic Redundancy Code.
    // TODO: Unused
    uint32_t crc;

    bytes_read = fread(&crc, 1, 4, file);
    if (bytes_read != 4)
        return FAILURE;

    flip(&crc, sizeof(crc));

    return SUCCESS;
}

// Flips N bytes them from big endian (PNG default
// endianness) to little endian.
// Assumes that buffer is well-defined with enough memory allocated.
void flip(void* buffer, size_t n_bytes) {
    // Now, flip the endianness. For every ith byte, we want to swap it with the
    // (n - i)th byte.
    if (n_bytes != 1) {
        uint8_t* casted_buffer = reinterpret_cast<uint8_t*>(buffer);

        for (uint32_t i = 0; i < n_bytes / 2; i++) {
            uint8_t first = casted_buffer[i];
            uint8_t second = casted_buffer[n_bytes - 1 - i];
            casted_buffer[i] = second;
            casted_buffer[n_bytes - 1 - i] = first;
        }
    }
}

} // namespace Graphics
} // namespace Engine