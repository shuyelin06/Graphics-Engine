#include "TextureBuilder.h"

#include "math/Vector4.h"

#include <assert.h>

namespace Engine
{
namespace Graphics
{
TextureColor::TextureColor()
    : data{0}
{
}
TextureColor::TextureColor(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a)
{
    auto& rgba = asType<TextureColor::UNormR8G8B8A8>();
    rgba.r = _r;
    rgba.g = _g;
    rgba.b = _b;
    rgba.a = _a;
}
TextureColor::TextureColor(float r)
{
    auto& f = asType<TextureColor::FloatR32>();
    f.r = r;
}

TextureBuilder::TextureBuilder(UINT width,
                               UINT height,
                               TextureLayout layout,
                               unsigned int numMips)
{
    reset(width, height, layout);
}

TextureBuilder::~TextureBuilder() = default;

const std::vector<uint8_t>& TextureBuilder::getData() const
{
    return data;
}
unsigned int TextureBuilder::getWidth(unsigned int mip) const
{
    return mips[mip].width;
}
unsigned int TextureBuilder::getHeight(unsigned int mip) const
{
    return mips[mip].height;
}
unsigned int TextureBuilder::getNumMips() const
{
    return mips.size();
}
TextureLayout TextureBuilder::getLayout() const
{
    return layout;
}

void TextureBuilder::generateMips()
{
    // Based on Mip 0, compute the number of mips we need.
    // This is basically the number of times we need to divide the width /
    // height to 1 such that both are 1.
    // We can compute as
    // floor( log_2 (width, height) ) + 1
    assert(!mips.empty());
    const MipLevel& mip0 = mips[0];
    const unsigned int totalMips =
        floor(log2f(max(mip0.width, mip0.height))) + 1;

    // Allocate space for each mip
    mips.resize(totalMips);

    size_t bufferSize = computeMipByteSize(mips[0]);
    for (int i = 1; i < mips.size(); i++)
    {
        mips[i].width = max(1, mips[i - 1].width / 2);
        mips[i].height = max(1, mips[i - 1].height / 2);
        bufferSize += computeMipByteSize(mips[i]);
    }

    // Figure out the mip data locations after resizing in case resizing causes
    // a reallocation in the heap.
    data.resize(bufferSize);
    mips[0].data = data.data();
    for (int i = 1; i < mips.size(); i++)
    {
        mips[i].data = mips[i - 1].data + computeMipByteSize(mips[i - 1]);
    }

    // Each mip is the 2x2 average of the previous
    for (int i = 1; i < mips.size(); i++)
    {
        const MipLevel& curMip = mips[i];
        const MipLevel& prevMip = mips[i - 1];

        for (int x = 0; x < curMip.width; x++)
        {
            for (int y = 0; y < curMip.height; y++)
            {
                int numSamples = 0;
                Vector4 rgba = Vector4(0, 0, 0, 0);

                auto accumulateSamples = [&rgba, &numSamples,
                                          this](int x, int y,
                                                const MipLevel& prevMip) {
                    if (hasTextureColor(prevMip, x, y))
                    {
                        numSamples++;

                        TextureColor::UNormR8G8B8A8& color =
                            getTextureColor(prevMip, x, y)
                                .asType<TextureColor::UNormR8G8B8A8>();
                        rgba.x += (float)color.r;
                        rgba.y += (float)color.g;
                        rgba.z += (float)color.b;
                        rgba.w += (float)color.a;
                    }
                };

                accumulateSamples(x * 2, y * 2, prevMip);
                accumulateSamples(x * 2 + 1, y * 2, prevMip);
                accumulateSamples(x * 2, y * 2 + 1, prevMip);
                accumulateSamples(x * 2 + 1, y * 2 + 1, prevMip);

                rgba = rgba / numSamples;

                TextureColor::UNormR8G8B8A8& texel =
                    getTextureColor(curMip, x, y)
                        .asType<TextureColor::UNormR8G8B8A8>();
                texel.r = (uint8_t)rgba.x;
                texel.g = (uint8_t)rgba.y;
                texel.b = (uint8_t)rgba.z;
                texel.a = (uint8_t)rgba.w;
            }
        }
    }
}

// SetColor:S
// Sets a pixel of the texture to some color value
void TextureBuilder::setColor(UINT x, UINT y, const TextureColor& rgba)
{
    assert(0 <= x && x < mips[0].width && 0 <= y && y < mips[0].height);

    const size_t byteSize = TextureLayoutByteSize(layout);
    void* addr = &data[byteSize * (y * mips[0].width + x)];
    memcpy(addr, &rgba.data, byteSize);
}

// Clear:
// Clears the texture, setting all of the RGBA pixels to a particular color.
void TextureBuilder::clear()
{
    memset(data.data(), 0, data.size());
}

// Reset:
// Resets the builder
void TextureBuilder::reset(unsigned int width,
                           unsigned int height,
                           TextureLayout layout)
{
    this->layout = layout;

    mips.resize(1);

    mips[0].width = width;
    mips[0].height = height;

    data.resize(computeMipByteSize(mips[0]));
    mips[0].data = data.data();

    clear();
}

size_t TextureBuilder::computeMipByteSize(const MipLevel& mipLevel)
{
    return TextureLayoutByteSize(layout) * mipLevel.width * mipLevel.height;
}

TextureColor& TextureBuilder::getTextureColor(const MipLevel& mipLevel,
                                              unsigned int x,
                                              unsigned int y)
{
    assert(x < mipLevel.width);
    assert(y < mipLevel.height);

    void* addr = mipLevel.data +
                 TextureLayoutByteSize(layout) * (y * mipLevel.height + x);
    return *static_cast<TextureColor*>(addr);
}

bool TextureBuilder::hasTextureColor(const MipLevel& mipLevel,
                                     unsigned int x,
                                     unsigned int y)
{
    return x < mipLevel.width && y < mipLevel.height;
}

// --- Atlas Builder ---
AtlasBuilder::AtlasBuilder(UINT atlas_width, UINT atlas_height)
    : TextureBuilder(atlas_width, atlas_height, TextureLayout::R8G8B8A8_UNORM)
{
    // Initialize my texture atlas
    atlas = new TextureAtlas(new Texture(nullptr, atlas_width, atlas_height));

    cur_region = nullptr;
}
AtlasBuilder::~AtlasBuilder() = default;

// AllocateRegion:
// Allocates a new region in the atlas for a new texture to be written to
const AtlasAllocation& AtlasBuilder::allocateRegion(UINT tex_width,
                                                    UINT tex_height)
{
    const UINT allocation_id = atlas->allocateTexture(tex_width, tex_height);
    const AtlasAllocation& allocation = atlas->getAllocation(allocation_id);

    cur_region = &allocation;

    return allocation;
}

// Accessors:
// Access properties of the atlas
UINT AtlasBuilder::getAtlasWidth() const
{
    return mips[0].width;
}
UINT AtlasBuilder::getAtlasHeight() const
{
    return mips[0].height;
}

// SetColor:
// Sets the color of a pixel relative to the current region
void AtlasBuilder::setColor(UINT x, UINT y, const TextureColor& rgba)
{
    // AllocateRegion should have been called before this
    assert(cur_region != nullptr);
    assert(x < cur_region->width);
    assert(y < cur_region->height);

    const UINT pixel_x = cur_region->x + x;
    const UINT pixel_y = cur_region->y + y;

    TextureBuilder::setColor(pixel_x, pixel_y, rgba);
}

// Clear:
// Clears the allocation region with an RGBA color
void AtlasBuilder::clear(const TextureColor& rgba)
{
    // AllocateRegion should have been called before this
    assert(cur_region != nullptr);

    const UINT width = cur_region->width;
    const UINT height = cur_region->height;

    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            setColor(x, y, rgba);
        }
    }
}

} // namespace Graphics
} // namespace Engine