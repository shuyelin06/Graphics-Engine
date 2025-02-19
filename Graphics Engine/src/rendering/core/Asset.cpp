#include "Asset.h"

#include <assert.h>

namespace Engine {
using namespace Math;

namespace Graphics {
// Material Constructor
Material::Material() {
    ka = Color(0.2f, 0.2f, 0.2f);
    kd = Color(0.8f, 0.8f, 0.8f);
    ks = Color(1.f, 1.f, 1.f);

    texture = std::string();
}

// Asset Class
// Represents a collection of meshes and materials. An object
// can register an asset to obtain a renderable entity.
Asset::Asset(Mesh* _mesh) : mesh(_mesh) {}
Asset::~Asset() = default;

// Access the meshes and materials in the asset.
const Mesh* Asset::getMesh() const { return mesh; }

} // namespace Graphics
} // namespace Engine