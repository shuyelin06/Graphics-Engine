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
Asset::Asset() = default;
Asset::~Asset() = default;

// AddMesh, AddMaterial
// Adds meshes and materials to the asset.
void Asset::addMesh(Mesh* mesh) { meshes.push_back(mesh); }

void Asset::addMaterial(Material* material) { materials.push_back(material); }

// Access the meshes and materials in the asset.
// Used in rendering.
std::vector<Mesh*>& Asset::getMeshes() { return meshes; }

std::vector<Material*>& Asset::getMaterials() { return materials; }

Mesh* Asset::getMesh(int mesh_index) { return meshes[mesh_index]; }
const Mesh* Asset::getMesh(int mesh_index) const { return meshes[mesh_index]; }

Material* Asset::getMaterial(int material_index) {
    return materials[material_index];
}

} // namespace Graphics
} // namespace Engine