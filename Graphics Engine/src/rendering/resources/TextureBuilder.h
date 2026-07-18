#pragma once
#pragma once

#include <vector>

#include "../core/TextureAtlas.h"

namespace Engine {
namespace Graphics {
class ResourceManagerImpl;

struct TextureColor {
    uint8_t data[4];

    struct UNormR8G8B8A8 {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;

        UNormR8G8B8A8& operator+=(const UNormR8G8B8A8& color) {
            r += color.r;
            g += color.g;
            b += color.b;
            a += color.a;
            return *this;
        }
    };
    struct FloatR32 {
        float r;
    };

    TextureColor();
    TextureColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    TextureColor(float r);

    template <typename T> T& asType() { return *reinterpret_cast<T*>(data); }
};

// TextureBuilder Class:
// Provides an interface for building textures manually.
// Pixels should be loaded in the range [0,255].
// The texture builder only supports the building of 8-bit RGBA channels.
class TextureBuilder {
    friend class ResourceManagerImpl;

  protected:
    struct MipLevel {
        uint8_t* data; // Pointer to the data vector
        unsigned int width = 0;
        unsigned int height = 0;
    };

    std::vector<uint8_t> data;
    std::vector<MipLevel> mips;
    TextureLayout layout;

  public:
    TextureBuilder(UINT width, UINT height, TextureLayout layout,
                   unsigned int numMips = 1);
    ~TextureBuilder();

    const std::vector<uint8_t>& getData() const;
    unsigned int getWidth(unsigned int mip = 0) const;
    unsigned int getHeight(unsigned int mip = 0) const;
    unsigned int getNumMips() const;
    TextureLayout getLayout() const;

    void generateMips();

    // Sets the color for a particular pixel
    void setColor(UINT x, UINT y, const TextureColor& rgba);
    void clear();

    // Resets the builder to the base mip and clears its data
    void reset(unsigned int width, unsigned int height, TextureLayout layout);

  private:
    size_t computeMipByteSize(const MipLevel& mipLevel);

    TextureColor& getTextureColor(const MipLevel& mipLevel, unsigned int x,
                                  unsigned int y);
    bool hasTextureColor(const MipLevel& mipLevel, unsigned int x,
                         unsigned int y);
};

// AtlasBuilder Class:
// An extended texture builder class, that supports writing to texture atlases.
// Can be used to build atlases of multiple textures together (reduce the total
// number of draw calls).
class AtlasBuilder : private TextureBuilder {
  private:
    using TextureBuilder::reset;

  protected:
    TextureAtlas* atlas;
    const AtlasAllocation* cur_region;

  public:
    // The constructor here sets the atlas size. This CANNOT be changed
    // after initialization
    AtlasBuilder(UINT atlas_width, UINT atlas_height);
    ~AtlasBuilder();

    // Get the atlas size
    UINT getAtlasWidth() const;
    UINT getAtlasHeight() const;

    // Allocate a new region on the atlas
    const AtlasAllocation& allocateRegion(UINT tex_width, UINT tex_height);

    // Sets the color for a particular pixel relative to the current allocation
    // region
    void setColor(UINT x, UINT y, const TextureColor& rgba);

    // Clears the allocation region with an RGBA color
    void clear(const TextureColor& rgba);
};

} // namespace Graphics
} // namespace Engine