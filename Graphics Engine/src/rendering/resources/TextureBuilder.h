#pragma once
#pragma once

#include <vector>

#include "../core/Texture.h"
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
    std::vector<uint8_t> data;
    UINT width, height;
    TextureLayout layout;

  public:
    TextureBuilder(UINT width, UINT height,
                   TextureLayout layout = TextureLayout::R8G8B8A8_UNORM);
    ~TextureBuilder();

    // Generates the renderable texture
    Texture* generate(ID3D11Device* device);
    Texture* generate(ID3D11Device* device, bool editable);
    // Updates the renderable texture with the builder's information
    void update(Texture* texture, ID3D11DeviceContext* context);

    // Sets the color for a particular pixel
    void setColor(UINT x, UINT y, const TextureColor& rgba);
    void clear();

    // Resets the builder
    void reset(unsigned int width, unsigned int height,
               TextureLayout layout = TextureLayout::R8G8B8A8_UNORM);
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

    // Generates the texture for the atlas and returns the atlas.
    TextureAtlas* generate(ID3D11Device* device);

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