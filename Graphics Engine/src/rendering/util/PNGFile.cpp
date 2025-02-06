#include "PNGFile.h"

#include <assert.h>
#include <vector>

#include <zlib.h>

#define SUCCESS 1
#define FAILURE 0

namespace Engine {
namespace Graphics {
static uint8_t PNG_MAGIC_BYTE[] = {0x89, 0x50, 0x4E, 0x47,
                                   0x0D, 0x0A, 0x1A, 0x0A};

PNGFile::PNGFile(const std::string& file_path) { path = file_path; }

// --- PNG File Writing ---
// Given a ID3D11Texture2D, writes its contents to a png file for exporting /
// reading externally.
bool PNGFile::writeTextureToFile(ID3D11Device* device,
                                 ID3D11DeviceContext* context,
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

    // For now, this will only work for DXGI_FORMAT_R8G8B8A8_UNORM.
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
    const int samples_per_row = mapped.RowPitch / 4;
    const int samples_per_column = mapped.DepthPitch / mapped.RowPitch;

    uint8_t* pixel_data = static_cast<uint8_t*>(mapped.pData);

    // We can access our texture contents with mapped.pData. We will now start
    // to generate our PNG file.
    FILE* file = fopen(path.c_str(), "wb");

    // Write the PNG magic byte
    fwrite(PNG_MAGIC_BYTE, sizeof(PNG_MAGIC_BYTE), 1, file);

    // Write the IHDR Chunk.
    const uint32_t width = samples_per_row;
    const uint32_t height = samples_per_column;

    {
        uint8_t IHDR_data[13];
        uint8_t IHDR_type[4] = {'I', 'H', 'D', 'R'};

        const uint8_t bit_depth = 8;
        const uint8_t color_type = 6;
        const uint8_t interlace = 0;
        const uint8_t compression = 0;

        uint32_t width_flipped = width;
        flip(&width_flipped, 4);
        memcpy(IHDR_data, &width_flipped, 4);

        uint32_t height_flipped = height;
        flip(&height_flipped, 4);
        memcpy(IHDR_data + 4, &height_flipped, 4);

        IHDR_data[8] = 8;  // Bit depth
        IHDR_data[9] = 6;  // Color type
        IHDR_data[10] = 0; // Compression
        IHDR_data[11] = 0; // Filter
        IHDR_data[12] = 0; // Interlace

        writePNGChunk(file, IHDR_type, IHDR_data, 13);
    }

    // Write the IDAT Chunk
    {
        uint8_t IDAT_type[4] = {'I', 'D', 'A', 'T'};
        uint8_t* IDAT_data = new uint8_t[4 * width * height];

        memcpy(IDAT_data, pixel_data, 4 * width * height);

        // Compress with zlib
        uint8_t* compressed_data = new uint8_t[4 * width * height];
        uLong raw_size = 4 * width * height;
        uLong compressed_size = 4 * width * height;
        compress((Bytef*)compressed_data, &compressed_size, IDAT_data,
                 4 * width * height);

        writePNGChunk(file, IDAT_type, compressed_data, compressed_size);

        delete[] IDAT_data;
        delete[] compressed_data;
    }

    // IEND Chunk
    {
        uint8_t IEND_type[4] = {'I', 'E', 'N', 'D'};

        writePNGChunk(file, IEND_type, NULL, 0);
    }

    fclose(file);

    // We unmap our resource and release it to free the memory.
    context->Unmap(staging_tex, 0);
    staging_tex->Release();

    return false;
}

// Reads a PNG Chunk. A PNG Chunk Consists of:
// 1) A 4-byte UINT giving the number of bytes in the data field
// 2) A 4-byte character sequence defining the chunk type
// 3) The bytes of data of the chunk (COMPRESSED?)
// 4) UNUSED: A 4-byte CRC calculated on fields (2) and (3), to check for data
// corruption. Returns 1 on success, 0 on failure.
void PNGFile::writePNGChunk(FILE* file, uint8_t type[4], uint8_t* data,
                   uint32_t data_size) {
    // Write the data length
    uint32_t length = data_size;
    flip(&length, 4);
    fwrite(&length, sizeof(data_size), 1, file);

    // Write the chunk type
    fwrite(type, sizeof(uint8_t), 4, file);

    // Write the chunk data
    if (data_size > 0) {
        fwrite(data, sizeof(uint8_t), data_size, file);
    }

    // Generate and write the CRC
    std::vector<uint8_t> type_and_data;
    type_and_data.resize(4 + data_size);

    type_and_data[0] = type[0];
    type_and_data[1] = type[1];
    type_and_data[2] = type[2];
    type_and_data[3] = type[3];

    if (data_size > 0) {
        memcpy(&type_and_data[0] + 4, data, data_size);
    }

    uint32_t crc = checksum32(&type_and_data[0], data_size + 4);
    fwrite(&crc, sizeof(uint32_t), 1, file);
}

// --- PNG File Reading ---
// Simple PNG File Parser based from https://pyokagan.name/blog/2019-10-14-png/
// http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html
// Returns a dynamically allocated pointer to a texture on success,
// and NULL on fail.
bool PNGFile::readTextureFromFile(TextureBuilder& builder) {
    // Open my PNG file, and return NULL if we failed to open the file
    FILE* file = fopen(path.c_str(), "rb");
    if (file == nullptr)
        return false;

    // First 8 bytes of the file must match the PNG magic number.
    {
        uint8_t read_data[8];
        fread(read_data, 1, 8, file);

        if (memcmp(read_data, PNG_MAGIC_BYTE, sizeof(PNG_MAGIC_BYTE)) != 0)
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
    assert(color_type == 2 || color_type == 6); // RGB (2) or RGBA (6) Channels
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
    const int BYTES_PER_PIXEL = (color_type == 2) ? 3 : 4;
    std::vector<uint8_t> decompressed;
    decompressed.resize(height * (1 + width * BYTES_PER_PIXEL));

    uLong decompressed_size = (uLong)decompressed.size();
    uLong raw_size = (uLong)raw_data.size();
    uncompress((Bytef*)&decompressed[0], &decompressed_size,
               (Bytef*)&raw_data[0], raw_size);

    // Read each scan-line, which has a width of (width * bytesPerPixel + 1),
    // where 1 accounts for the filtering method.
    for (uint32_t y = 0; y < height; y++) {
        uint8_t filter_method =
            decompressed[y * (width * BYTES_PER_PIXEL + 1) + 0];

        // Filter Method 0: No filtering
        // Filter Method 1: Recon(x) = Filter(x) + Recon(a)
        if (filter_method == 1) {
            uint8_t prev_r = 0;
            uint8_t prev_g = 0;
            uint8_t prev_a = 0;
            uint8_t prev_b = 0;

            for (uint32_t x = 0; x < width; x++) {
                uint8_t r = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                         (x * BYTES_PER_PIXEL) + 1];
                uint8_t g = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                         (x * BYTES_PER_PIXEL) + 2];
                uint8_t b = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                         (x * BYTES_PER_PIXEL) + 3];

                decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                             (x * BYTES_PER_PIXEL) + 1] = r + prev_r;
                prev_r = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                      (x * BYTES_PER_PIXEL) + 1];
                decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                             (x * BYTES_PER_PIXEL) + 2] = g + prev_g;
                prev_g = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                      (x * BYTES_PER_PIXEL) + 2];
                decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                             (x * BYTES_PER_PIXEL) + 3] = b + prev_b;
                prev_b = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                      (x * BYTES_PER_PIXEL) + 3];

                if (color_type == 6) {
                    uint8_t a = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                             (x * BYTES_PER_PIXEL) + 4];
                    decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                 (x * BYTES_PER_PIXEL) + 4] = a + prev_a;
                    prev_a = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                          (x * BYTES_PER_PIXEL) + 4];
                }
                
            }
        }

        assert(filter_method == 0 || filter_method == 1);

        for (uint32_t x = 0; x < width; x++) {
            uint8_t r = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                     (x * BYTES_PER_PIXEL) + 1];
            uint8_t g = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                     (x * BYTES_PER_PIXEL) + 2];
            uint8_t b = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                     (x * BYTES_PER_PIXEL) + 3];
            uint8_t a = 255;
            
            if (color_type == 6) {
                a = decompressed[y * (width * BYTES_PER_PIXEL + 1) +
                                 (x * BYTES_PER_PIXEL) + 4];
            }
            
            builder.setColor(x, y, {r, g, b, a});
        }
    }

    return true;
}

// Reads a PNG Chunk. A PNG Chunk Consists of:
// 1) A 4-byte UINT giving the number of bytes in the data field
// 2) A 4-byte character sequence defining the chunk type
// 3) The bytes of data of the chunk
// 4) A 4-byte CRC calculated on fields (2) and (3), to check for data
//    corruption. Returns 1 on success, 0 on failure.
uint8_t PNGFile::readPNGChunk(FILE* file, PNGChunk& chunk) {
    size_t bytes_read = 0;

    // Read the 4-byte length of the chunk.
    uint32_t length;

    bytes_read = fread(&length, 1, 4, file);
    if (bytes_read != 4) // Fail if 4 bytes were not read
        return FAILURE;

    flip(&length, sizeof(length));

    // Read the 4-byte chunk type and chunk data together. We do this so that
    // we can compute the CRC on the data as an integrity check.
    chunk.chunk_data.resize(4 + length);
    bytes_read = fread(&(chunk.chunk_data[0]), 1, 4 + length, file);

    if (bytes_read != 4 + length)
        return FAILURE;

    // Read the 4-byte Cyclic Redundancy Code.
    uint32_t crc;

    bytes_read = fread(&crc, 1, 4, file);
    if (bytes_read != 4)
        return FAILURE;

    flip(&crc, sizeof(crc));

    // Compute our own CRC, and check if they match.
    uint32_t computed_crc = checksum32(&(chunk.chunk_data[0]), 4 + length);

    assert(crc == computed_crc);
    if (crc != computed_crc)
        return FAILURE;

    // If they do, parse out our chunk type and data.
    chunk.chunk_type[0] = chunk.chunk_data[0];
    chunk.chunk_type[1] = chunk.chunk_data[1];
    chunk.chunk_type[2] = chunk.chunk_data[2];
    chunk.chunk_type[3] = chunk.chunk_data[3];
    chunk.chunk_data.erase(chunk.chunk_data.begin(),
                           chunk.chunk_data.begin() + 4);

    return SUCCESS;
}

// Flips the endianness of n bytes.
// Assumes that buffer is well-defined with enough memory allocated.
void PNGFile::flip(void* buffer, size_t n_bytes) {
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

// Generates the 32-bit CRC for the data from a PNG file.
// A CRC serves as bit hash for an arbitrary stream of bits.
// For understanding, this function will use booleans to represent individual
// bits.
// More info: https://www.geeksforgeeks.org/modulo-2-binary-division/#
// https://stackoverflow.com/questions/2587766/how-is-a-crc32-checksum-calculated
uint32_t PNGFile::checksum32(uint8_t* _data, size_t size) {
    // PNG uses the following polynomial for the checksum:
    // x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1
    // Where the presence of the x^i power indicates if that bit is flipped (1).
    // This translates to: 1 0000 0100 1100 0001 0001 1101 1011 0111
    // We often will omit the highest term, x^32, to get a 32 bit number
    // (hex): 0x04C11DB7
    bool crc_code[32] = {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1,
                         0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1};

    // We now parse our data into a boolean array. Each byte will be reversed as
    // it is written into the array, as the specification requires that bits be
    // processed from least significant to most significant. The last 32 bits
    // will be our remainder, set initially to 0's.
    bool* data = new bool[size * 8 + 32];

    for (int i = 0; i < size; i++) {
        // We reverse each bit written to the boolean array
        for (int bit = 0; bit < 8; bit++) {
            const uint8_t mask = (1 << bit);
            data[i * 8 + bit] = (_data[i] & mask) == mask;
        }
    }
    // Initialize remainder (last 32 bits) to all 0s
    for (int bit = 0; bit < 32; bit++) {
        data[size * 8 + bit] = false;
    }
    // XOR the first 4 bytes with 0xFFFFFFFF
    for (int bit = 0; bit < 32; bit++) {
        data[bit] = !data[bit];
    }

    // We will now calculate our hash. This is essentially long division,
    // where our crc is a divisor and we want to divide the data. However,
    // our "division" operation is XOR.
    // We use our polynomial, and anytime the most significant bit is a 1, we
    // divide by our polynomial.
    for (int i = 0; i < size * 8; i++) {
        // If current bit is flipped, XOR by our crc on the next 32 bits.
        if (data[i]) {
            data[i] = false;
            for (int j = 0; j < 32; j++) {
                if (crc_code[j])
                    data[i + j + 1] = !data[i + j + 1];
            }
        }
    }

    // Our result is the last 32 bits. We transform this to a uint32_t integer,
    // and XOR it with 0xFFFFFFFF by the specifications.
    uint32_t hash = 0;

    for (int i = 0; i < 32; i++) {
        hash |= data[size * 8 + i] << i;
    }

    hash ^= 0xFFFFFFFF;

    return hash;
}

} // namespace Graphics
} // namespace Engine