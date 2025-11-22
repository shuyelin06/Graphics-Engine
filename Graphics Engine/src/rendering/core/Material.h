#pragma once

#include <memory>

#include "Texture.h"

namespace Engine {
namespace Graphics {
// --- Material ---
// Renderable properties of a mesh. A mesh is rendered with a material,
// which determines how it is rendered.

// Normalized [0,1] coordinates into a material texture atlas.
struct TextureRegion {
    float x, y, width, height;
};

struct Material {
    std::shared_ptr<Texture> colormap;
};

} // namespace Graphics
} // namespace Engine