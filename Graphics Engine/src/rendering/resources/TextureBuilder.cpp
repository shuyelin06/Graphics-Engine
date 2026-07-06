#include "TextureBuilder.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
TextureColor::TextureColor() : data{0} {}
TextureColor::TextureColor(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) {
    auto& rgba = asType<TextureColor::UNormR8G8B8A8>();
    rgba.r = _r;
    rgba.g = _g;
    rgba.b = _b;
    rgba.a = _a;
}
TextureColor::TextureColor(float r) {
    auto& f = asType<TextureColor::FloatR32>();
    f.r = r;
}

TextureBuilder::TextureBuilder(UINT width, UINT height, TextureLayout layout)
    : width(width), height(height), layout(layout) {
    reset(width, height, layout);
}

TextureBuilder::~TextureBuilder() = default;

const std::vector<uint8_t>& TextureBuilder::getData() const { return data; }
unsigned int TextureBuilder::getWidth() const { return width; }
unsigned int TextureBuilder::getHeight() const { return height; }
TextureLayout TextureBuilder::getLayout() const { return layout; }

// SetColor:S
// Sets a pixel of the texture to some color value
void TextureBuilder::setColor(UINT x, UINT y, const TextureColor& rgba) {
    assert(0 <= x && x < width && 0 <= y && y < height);

    const size_t byteSize = TextureLayoutByteSize(layout);
    void* addr = &data[byteSize * (y * width + x)];
    memcpy(addr, &rgba.data, byteSize);
}

// Clear:
// Clears the texture, setting all of the RGBA pixels to a particular color.
void TextureBuilder::clear() { memset(data.data(), 0, data.size()); }

// Reset:
// Resets the builder
void TextureBuilder::reset(unsigned int _width, unsigned int _height,
                           TextureLayout _layout) {
    width = _width;
    height = _height;
    layout = _layout;

    data.resize(TextureLayoutByteSize(layout) * _width * _height);
    clear();
}

// --- Atlas Builder ---
AtlasBuilder::AtlasBuilder(UINT atlas_width, UINT atlas_height)
    : TextureBuilder(atlas_width, atlas_height) {
    // Initialize my texture atlas
    atlas = new TextureAtlas(new Texture(nullptr, atlas_width, atlas_height));

    cur_region = nullptr;
}
AtlasBuilder::~AtlasBuilder() = default;

// AllocateRegion:
// Allocates a new region in the atlas for a new texture to be written to
const AtlasAllocation& AtlasBuilder::allocateRegion(UINT tex_width,
                                                    UINT tex_height) {
    const UINT allocation_id = atlas->allocateTexture(tex_width, tex_height);
    const AtlasAllocation& allocation = atlas->getAllocation(allocation_id);

    cur_region = &allocation;

    return allocation;
}

// Accessors:
// Access properties of the atlas
UINT AtlasBuilder::getAtlasWidth() const { return width; }
UINT AtlasBuilder::getAtlasHeight() const { return height; }

// SetColor:
// Sets the color of a pixel relative to the current region
void AtlasBuilder::setColor(UINT x, UINT y, const TextureColor& rgba) {
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
void AtlasBuilder::clear(const TextureColor& rgba) {
    // AllocateRegion should have been called before this
    assert(cur_region != nullptr);

    const UINT width = cur_region->width;
    const UINT height = cur_region->height;

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            setColor(x, y, rgba);
        }
    }
}

} // namespace Graphics
} // namespace Engine