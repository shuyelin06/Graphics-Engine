#include "ResourceManager.h"

#include <fstream>
#include <iostream>

#include <map>
#include <string.h>
#include <vector>

#include <assert.h>

#include "datamodel/Terrain.h"
#include "math/Vector2.h"
#include "math/Vector3.h"

#include "GLTFFile.h"
#include "OBJFile.h"
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
void ResourceManager::initialize() {
    // Create my samplers
    shadowmap_sampler = LoadShadowMapSampler();
    mesh_sampler = LoadMeshTextureSampler();

    // Prepare my builders
    TextureBuilder::device = device;

    // Stores an atlas of material colors to avoid the need for rebinds later
    AtlasBuilder atlas_builder = AtlasBuilder(4096, 4096);

    // --- Load Assets Here ---
    // Currently supported: GLTF

    // LoadAssetFromGLTF("TestAsset", "data/Testing.glb", atlas_builder);
    // Capybara by Poly by Google [CC-BY] via Poly Pizza
    LoadAssetFromGLTF("Capybara", "data/Capybara.glb", atlas_builder);

    // LoadAssetFromGLTF("TexturedCube", "data/TexturedCube.glb", atlas_builder);

    // Dingus the cat by alwayshasbean [CC-BY] via Poly Pizza
    //LoadAssetFromGLTF("Dingus", "data/Dingus the cat.glb", atlas_builder);

    // Fox by Quaternius
    // TODO: Add animation support
    LoadAssetFromGLTF("Fox", "data/Fox.glb", atlas_builder);

    color_atlas = atlas_builder.generate();
}

uint16_t ResourceManager::registerAsset(const std::string& name, Asset* asset) {
    const uint16_t id = (uint16_t)assets.size();

    asset_map[name] = id;
    assets.push_back(asset);

    return id;
}

// CreateMeshBuilder:
// Creates and returns a mesh builder
MeshBuilder* ResourceManager::createMeshBuilder() {
    return new MeshBuilder(device);
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

const Texture* ResourceManager::getColorAtlas() {
    return color_atlas->getTexture();
}

ID3D11SamplerState* ResourceManager::getShadowMapSampler() {
    return shadowmap_sampler;
}

ID3D11SamplerState* ResourceManager::getMeshSampler() { return mesh_sampler; }

// LoadAssetFromGLTF:
// Uses the GLTFFile interface to load an asset from a GLTF file
bool ResourceManager::LoadAssetFromGLTF(const std::string& asset_name,
                                        const std::string& path,
                                        AtlasBuilder& tex_builder) {
    MeshBuilder mesh_builder = MeshBuilder(device);

    GLTFFile gltf_file = GLTFFile(path);
    Asset* asset = gltf_file.readFromFile(mesh_builder, tex_builder);

    if (asset != nullptr) {
        registerAsset(asset_name, asset);
        return true;
    } else
        return false;
}

// LoadTextureFromPNG:
// Uses the PNGFile interface to load a texture from a PNG file
bool ResourceManager::LoadTextureFromPNG(TextureBuilder& builder,
                                         std::string path, std::string file) {
    PNGFile png_file = PNGFile(path + file);
    return png_file.readPNGData(builder);
}

// WriteTextureToPNG:
// Uses the PNGFile interface to write a texture to a PNG file
bool ResourceManager::WriteTextureToPNG(ID3D11Texture2D* texture,
                                        std::string path, std::string file) {
    PNGFile png_file = PNGFile(path + file);
    return png_file.writePNGData(device, context, texture);
}

// Helper parsing functions

Asset* ResourceManager::LoadAssetFromOBJ(const std::string& path,
                                         const std::string& objFile) {
    MeshBuilder mesh_builder = MeshBuilder(device);
    TextureBuilder texture_builder(0, 0);

    OBJFile obj_file = OBJFile(path, objFile);
    return obj_file.readAssetFromFile(mesh_builder, texture_builder);
}

// Hard-Coded Cube Creator
// Used in debugging
Asset* ResourceManager::LoadCube() {
    MeshBuilder builder = MeshBuilder(device);
    builder.addCube(Vector3(0, 0, 0), Quaternion(), 1.f);

    Asset* cube = new Asset();
    cube->addMesh(builder.generate());

    return cube;
}

// Load___Sampler:
// Create Texture Samplers!
ID3D11SamplerState* ResourceManager::LoadShadowMapSampler() {
    ID3D11SamplerState* sampler;

    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter =
        D3D11_FILTER_MIN_MAG_MIP_LINEAR; // Linear Filtering for PCF
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.BorderColor[0] = 0.f;
    sampler_desc.BorderColor[1] = 0.f;
    sampler_desc.BorderColor[2] = 0.f;
    sampler_desc.BorderColor[3] = 0.f;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = 1.0f;

    device->CreateSamplerState(&sampler_desc, &sampler);
    assert(sampler != NULL);

    return sampler;
}

ID3D11SamplerState* ResourceManager::LoadMeshTextureSampler() {
    ID3D11SamplerState* sampler;

    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    device->CreateSamplerState(&sampler_desc, &sampler);
    assert(sampler != NULL);

    return sampler;
}
} // namespace Graphics
} // namespace Engine