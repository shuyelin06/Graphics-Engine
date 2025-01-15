#pragma once

#include <vector>

#include "Texture.h"

namespace Engine {
namespace Graphics {
// TextureBuilder Class:
// Provides an interface for creating Textures.
// Only supports R8G8B8A8 textures.
struct TextureColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

class TextureBuilder {
  private:
    // Device interface for creating GPU resources.
    // Set by AssetManager
    friend class AssetManager;
    static ID3D11Device* device;

    // Data for the texture
    unsigned int pixel_width;
    unsigned int pixel_height;

    std::vector<TextureColor> data;

  public:
    TextureBuilder(UINT _width, UINT _height);
    ~TextureBuilder();

    // Generates the renderable texture
    Texture* generate();

    // Sets the color for a particular pixel
    void setColor(UINT x, UINT y, const TextureColor& rgba);

    // Clears the texture with an rgba color
    void clear(const TextureColor& rgba);

    // Resets the builder
    void reset(unsigned int width, unsigned int height);
};

} // namespace Graphics
} // namespace Engine