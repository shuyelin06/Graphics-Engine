#pragma once

#include <vector>

#include "math/Vector2.h"
#include "rendering/Direct3D11.h"
#include "rendering/core/Texture.h"

// Enables generation of a "viewing" texture
// that lets you see the allocations
#if defined(_DEBUG)
#define TOGGLE_ALLOCATION_VIEW
#endif

namespace Engine {
using namespace Math;

namespace Graphics {

typedef unsigned int UINT;

// TextureAllocation Struct:
// Stores the data regarding where an individual "texture" is
// located in the atlas. Can be used, with texture coordinates (x,y),
// x,y in [0,1], to reference the actual texel coordinate in the TextureAtlas.
struct TextureAllocation {
    UINT x, y; // Top-left corner in atlas
    UINT width, height;

    TextureAllocation(UINT x, UINT y, UINT w, UINT h);
    TextureAllocation();

    UINT area() const;
};

// TextureAtlas Class:
// Stores a collection of 2D textures, all packed into 1 single GPU texture.
// Doing this lets us save continuous re-bind calls for textures, and let us
// ultimately pass more individual "textures" to the graphics pipeline for use.
// Atlas coordinates are given as (0,0) on the top-left, (1,1) on the
// bottom-right (note that Y grows top-down).
class TextureAtlas {
  private:
    Texture texture;

    std::vector<TextureAllocation> allocations;

    // Used in the 2D Rectangle Packing (see allocateTexture())
    std::vector<TextureAllocation> open_regions;

  public:
    TextureAtlas(UINT width, UINT height);
    ~TextureAtlas();

    void initialize();

    // Use a 2D Rectangle Packing algorithm to pack the
    // allocated textures tightly.
    const TextureAllocation& getAllocation(UINT index) const;
    Vector2 getAtlasCoordinates(UINT texture, Vector2 tex_coords);

    UINT allocateTexture(UINT tex_width, UINT tex_height);

#if defined(TOGGLE_ALLOCATION_VIEW)
    Texture* getAllocationView();
#endif
};

} // namespace Graphics
} // namespace Engine