#include "Asset.h"

#include <assert.h>

namespace Engine {
using namespace Math;

namespace Graphics {
// Material Constructor
Material::Material() {
    base_color = Color(1, 1, 1);

    diffuse_factor = 0.5f;
}

// Asset Class
// Represents a collection of meshes and materials. An object
// can register an asset to obtain a renderable entity.
Asset::Asset() = default;
Asset::~Asset() = default;

// Create an Asset
void Asset::addMesh(Mesh* mesh) { meshes.push_back(mesh); }

// Access the meshes and materials in the asset.
const std::vector<Mesh*>& Asset::getMeshes() const { return meshes; }

const Mesh* Asset::getMesh(int index) const { return meshes[index]; }

} // namespace Graphics
} // namespace Engine