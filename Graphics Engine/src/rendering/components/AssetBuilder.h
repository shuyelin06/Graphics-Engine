#pragma once

#include <vector>

#include "Asset.h"

namespace Engine
{
namespace Graphics
{
    // AssetBuilder Class:
    // Enables creation of assets, that can be read into
    class AssetBuilder {
    
    };

    // MeshBuilder Class:
    // Enables creation of meshes
    
    // TextureBuilder Class:
    // Provides an interface for creating Textures.
    // Only supports R8G8B8A8 textures.
    struct TextureColor 
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    class TextureBuilder
    {
    private:
        unsigned int pixel_width;
        unsigned int pixel_height;

        std::vector<TextureColor> data;

    public:
        TextureBuilder(unsigned int _width, unsigned int _height);
        ~TextureBuilder();

        // Generates the renderable texture
        Texture* generate(ID3D11Device* device);

        // Sets the color for a particular pixel
        void setColor(unsigned int x, unsigned int y, const TextureColor& rgba);

        // Clears the texture with an rgba color
        void clear(const TextureColor& rgba);

        // Resets the builder
        void reset(unsigned int width, unsigned int height);
    };
}
}