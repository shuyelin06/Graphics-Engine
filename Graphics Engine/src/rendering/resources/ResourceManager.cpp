#include "ResourceManager.h"

#include <fstream>
#include <iostream>

#include <map>
#include <string.h>
#include <vector>

#include <assert.h>

#include "math/Vector2.h"
#include "math/Vector3.h"

#include "GLTFFile.h"
#include "PNGFile.h"

using namespace std;

namespace Engine {
using namespace Math;

namespace Graphics {
ResourceManager::ResourceManager(ID3D11Device* _device,
                                 ID3D11DeviceContext* _context) {
    device = _device;
    context = _context;
}
ResourceManager::~ResourceManager() = default;

// Initialize:
// Loads assets into the asset manager.
void ResourceManager::initializeResources() {
    // Stores an atlas of material colors to avoid the need for rebinds later
    AtlasBuilder atlas_builder = AtlasBuilder(4096, 4096);

    // --- Load Assets Here ---
    LoadCube();

    // Currently supported: GLTF

    // LoadAssetFromGLTF("TestAsset", "data/Testing.glb", atlas_builder);
    // Capybara by Poly by Google [CC-BY] via Poly Pizza
    LoadAssetFromGLTF("Capybara", "data/Capybara.glb", atlas_builder);

    // LoadAssetFromGLTF("TexturedCube", "data/TexturedCube.glb",
    // atlas_builder);

    // Dingus the cat by alwayshasbean [CC-BY] via Poly Pizza
    // LoadAssetFromGLTF("Dingus", "data/Dingus the cat.glb", atlas_builder);

    // Fox by Quaternius
    LoadAssetFromGLTF("Fox", "data/Fox.glb", atlas_builder);

    // Man by Quaternius
    LoadAssetFromGLTF("Man", "data/Man.glb", atlas_builder);

    // Tree
    LoadAssetFromGLTF("Tree", "data/Tree.glb", atlas_builder);

    color_atlas = atlas_builder.generate(device);
}

uint16_t ResourceManager::registerAsset(const std::string& name, Asset* asset) {
    const uint16_t id = (uint16_t)assets.size();

    asset_map[name] = id;
    assets.push_back(asset);

    return id;
}

// Get Resources:
// Return resources by name.
Asset* ResourceManager::getAsset(const std::string& name) {
    if (asset_map.contains(name))
        return getAsset(asset_map[name]);
    else
        return nullptr;
}

Asset* ResourceManager::getAsset(uint16_t id) { return assets[id]; }

Texture* ResourceManager::getTexture(const std::string& name) {
    if (texture_map.contains(name))
        return getTexture(texture_map[name]);
    else
        return nullptr;
}
Texture* ResourceManager::getTexture(uint16_t id) { return textures[id]; }

const Texture* ResourceManager::getColorAtlas() {
    return color_atlas->getTexture();
}

// LoadAssetFromGLTF:
// Uses the GLTFFile interface to load an asset from a GLTF file
bool ResourceManager::LoadAssetFromGLTF(const std::string& asset_name,
                                        const std::string& path,
                                        AtlasBuilder& tex_builder) {
    MeshBuilder mesh_builder = MeshBuilder();
    GLTFFile gltf_file = GLTFFile(path);
    Asset* asset =
        gltf_file.readFromFile(mesh_builder, tex_builder, device, context);

    if (asset != nullptr) {
        registerAsset(asset_name, asset);
        return true;
    } else
        return false;
}

// LoadTextureFromPNG:
// Uses the PNGFile interface to load a texture from a PNG file
bool ResourceManager::LoadTextureFromPNG(const std::string& tex_name,
                                         const std::string& path,
                                         TextureBuilder& builder) {
    PNGFile png_file = PNGFile(path);
    png_file.readPNGData(builder);
    Texture* tex = builder.generate(device);

    if (tex != nullptr) {
        const uint16_t index = textures.size();
        textures.push_back(tex);
        texture_map[tex_name] = index;

        return true;
    } else
        return false;
}

// WriteTextureToPNG:
// Uses the PNGFile interface to write a texture to a PNG file
bool ResourceManager::WriteTextureToPNG(ID3D11Texture2D* texture,
                                        std::string path, std::string file) {
    PNGFile png_file = PNGFile(path + file);
    return png_file.writePNGData(device, context, texture);
}

// Hard-Coded Cube Creator
// Used in debugging
bool ResourceManager::LoadCube() {
    MeshBuilder builder = MeshBuilder();
    builder.addLayout(POSITION);

    builder.addCube(Vector3(0, 0, 0), Quaternion(), 1.f);

    Asset* cube = new Asset();
    cube->addMesh(builder.generateMesh(device));

    return registerAsset("Cube", cube);
}

} // namespace Graphics
} // namespace Engine