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

#include "rendering/util/GLTFFile.h"
#include "rendering/util/OBJFile.h"
#include "rendering/util/PNGFile.h"

#include "math/PerlinNoise.h"

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
    // Prepare my builders
    TextureBuilder::device = device;

    // Create my textures
    TextureBuilder tex_builder = TextureBuilder(10, 10);

    // textures[Test] = tex_builder.generate();


    // LoadTextureFromPNG(tex_builder, "data/", "test.png");
    //  textures[Test2] = tex_builder.generate();

    LoadTextureFromPNG(tex_builder, "data/", "grass.png");
    textures["TerrainGrass"] = tex_builder.generate();

    LoadTextureFromPNG(tex_builder, "data/", "Capybara_BaseColor.png");
    textures["CapybaraTex"] = tex_builder.generate();

    PNGFile test = PNGFile("data/Capybara_BaseColor.png");
    test.readPNGData(tex_builder);

    // Create my samplers
    shadowmap_sampler = LoadShadowMapSampler();
    mesh_sampler = LoadMeshTextureSampler();

    // Create my assets
    MeshBuilder mesh_builder = MeshBuilder(device);

    GLTFFile gltf = GLTFFile("data/Testing.glb");
    gltf.readFromFile();

    registerAsset("Cube", LoadCube(mesh_builder));
    // Fox by Jake Blakeley [CC-BY] via Poly Pizza
    registerAsset("Fox", LoadAssetFromOBJ("data/", "model.obj"));
    // Capybara by Poly by Google [CC-BY] via Poly Pizza
    registerAsset("Capybara", LoadAssetFromOBJ("data/", "Capybara.obj"));
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

Texture* ResourceManager::getTexture(const std::string& name) {
    if (textures.contains(name))
        return textures[name];
    else
        return nullptr;
}

ID3D11SamplerState* ResourceManager::getShadowMapSampler() {
    return shadowmap_sampler;
}

ID3D11SamplerState* ResourceManager::getMeshSampler() { return mesh_sampler; }

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
Asset* ResourceManager::LoadCube(MeshBuilder& builder) {
    builder.reset();
    builder.addCube(Vector3(0, 0, 0), Quaternion(), 1.f);

    Asset* cube = new Asset(builder.generate());
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