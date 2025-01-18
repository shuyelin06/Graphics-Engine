#include "TextureAtlas.h"

#include <assert.h>

#include "rendering/core/TextureBuilder.h"

#if defined(TOGGLE_ALLOCATION_VIEW)
#include "math/Compute.h"
#include "rendering/core/TextureBuilder.h"

using namespace Engine::Math;
#endif

namespace Engine {
namespace Graphics {
TextureAllocation::TextureAllocation() = default;
TextureAllocation::TextureAllocation(UINT _x, UINT _y, UINT _w, UINT _h) {
    x = _x;
    y = _y;

    width = _w;
    height = _h;
}

// Area:
// Returns the pixel area of the allocated region
UINT TextureAllocation::area() const { return width * height; }

TextureAtlas::TextureAtlas(UINT _width, UINT _height)
    : allocations(), open_regions() {
    texture.width = _width;
    texture.height = _height;

    open_regions.push_back(TextureAllocation(0, 0, _width, _height));
}
TextureAtlas::~TextureAtlas() = default;

// Initialize:
// Generates the GPU texture resource
void TextureAtlas::initialize() {
    // TODO
}

// GetAllocation:
// Returns the allocation for a texture
const TextureAllocation& TextureAtlas::getAllocation(UINT index) const {
    return allocations[index];
}

// GetAtlasCoordinates:
// Transform the texture coordinates into atlas coordinates.
// Ignores texture addressing: TODO
Vector2 TextureAtlas::getAtlasCoordinates(UINT texture, Vector2 tex_coords) {
    const TextureAllocation& allocation = allocations[texture];

    const float x = allocation.x + tex_coords.u * allocation.width;
    const float y = allocation.y + tex_coords.v * allocation.height;

    return Vector2(x, y);
}

// AllocateTexture:
// Allocates space for a texture of specified width and height in the atlas.
// Uses a 2D Rectangle Packing Algorithm to do this tightly.
// This is a open area of research, and the algorithm could be iterated on.
// Returns the index of the allocation in the atlas, as a unique ID to the
// allocation.
// This uses a simple tree-search approach, where we consider the texture space
// to be divided along the edge of each allocation. Each region can have a
// texture allocated to it. Thus, all we do is search the available regions and
// find the one that is the best fit for what we need.
UINT TextureAtlas::allocateTexture(UINT tex_width, UINT tex_height) {
    UINT index;
    bool allocated = false;

    // To make an allocation, we will search all open regions
    // and choose the smallest one that can fit the allocation we need.
    TextureAllocation* smallest_region = nullptr;

    for (TextureAllocation& open_region : open_regions) {
        // If the texture can be contained within the region, choose
        // the smallest allocation region possible
        if (tex_width <= open_region.width &&
            tex_height <= open_region.height) {
            if (smallest_region == nullptr ||
                open_region.area() < smallest_region->area()) {
                smallest_region = &open_region;
            }
        }
    }

    // Now, make our allocation on this region on the top-left corner,
    // and split it into smaller open subregions.
    if (smallest_region != nullptr) {
        allocated = true;

        // Generate my texture allocation
        const TextureAllocation allocation = TextureAllocation(
            smallest_region->x, smallest_region->y, tex_width, tex_height);
        index = allocations.size();
        allocations.push_back(allocation);

        // Divide our open region to remove the allocated part
        // A | B
        // -----
        // C | D
        // Case 1: Texture completely fills the entire region
        if (tex_width == smallest_region->width &&
            tex_height == smallest_region->height) {
            // Remove the allocated region entirely
            smallest_region->width = 0;
            smallest_region->height = 0;
        }
        // Case 2: Texture width matches, not height
        else if (tex_width == smallest_region->width &&
                 tex_height < smallest_region->height) {
            // We have subregions C and D left. We modify smallest_region
            // to reflect the change in open space.
            smallest_region->y = allocation.y + allocation.height;
            smallest_region->height =
                smallest_region->height - allocation.height;
        }
        // Case 3: Texture height matches, not width
        else if (tex_width < smallest_region->width &&
                 tex_height == smallest_region->height) {
            // We have subregions B and D left. We modify smallest_region
            // to reflect the change in open space.
            smallest_region->x = allocation.x + allocation.width;
            smallest_region->width = smallest_region->width - allocation.width;
        }
        // Case 4: Texture width and height both do not match
        else {
            const TextureAllocation* access = smallest_region;

            // We have subregions B,C,D. To allow for larger allocations later, we will merge B/D into one
            // Allocation for C
            const TextureAllocation alloc_C = TextureAllocation(
                access->x, access->y + allocation.height, allocation.width,
                access->height - allocation.height);

            // Allocation for B,D
            smallest_region->x = allocation.x + allocation.width;
            smallest_region->width = smallest_region->width - allocation.width;

            open_regions.push_back(alloc_C);
        }
    }

    // Throw exception if allocation failed
    assert(allocated);

    return index;
}

#if defined(TOGGLE_ALLOCATION_VIEW)
Texture* TextureAtlas::getAllocationView() {
    TextureBuilder builder = TextureBuilder(texture.width, texture.height);

    for (const TextureAllocation& alloc : allocations) {
        TextureColor color;
        color.r = Compute::Random(0, 255);
        color.g = Compute::Random(0, 255);
        color.b = Compute::Random(0, 255);
        color.a = 255;

        for (int x = 0; x < alloc.width; x++) {
            for (int y = 0; y < alloc.height; y++) {
                builder.setColor(x + alloc.x, y + alloc.y, color);
            }
        }
    }

    return builder.generate();
}
#endif
} // namespace Graphics
} // namespace Engine