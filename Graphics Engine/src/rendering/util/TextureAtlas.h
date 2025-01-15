#pragma once

#include <vector>

#include "rendering/Direct3D11.h"
#include "rendering/core/Texture.h"
#include "math/Vector2.h"

namespace Engine
{
using namespace Math;

namespace Graphics 
{

typedef unsigned int UINT;

// TextureAllocation Struct:
// Stores the data regarding where an individual "texture" is
// located in the atlas. Can be used, with texture coordinates (x,y),
// x,y in [0,1], to reference the actual texel coordinate in the TextureAtlas.
struct TextureAllocation {
    UINT x, y; // Top-left corner in atlas
    UINT width, height;
};

// TextureAtlas Class:
// Stores a collection of 2D textures, all packed into 1 single GPU texture.
// Doing this lets us save continuous re-bind calls for textures, and let us ultimately pass more 
// individual "textures" to the graphics pipeline for use.
// Atlas coordinates are given as (0,0) on the top-left, (1,1) on the bottom-right (note
// that Y grows top-down).
class TextureAtlas {
private:
    ID3D11Texture2D* texture;

    std::vector<TextureAllocation> textures;
    UINT width, height;

    // Used for naive rectangle packing
    UINT x_alloc_bound, y_alloc_bound;

public:
    TextureAtlas(UINT width, UINT height);
    ~TextureAtlas();

    void initialize(ID3D11Device* device, DXGI_FORMAT format, UINT bind_flags);

    // Use a 2D Rectangle Packing algorithm to pack the
    // allocated textures tightly.
    const TextureAllocation& getAllocation(UINT index) const;
    Vector2 getAtlasCoordinates(UINT texture, Vector2 tex_coords);

    UINT allocateTexture(UINT tex_width, UINT tex_height);
};

}}