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
static ID3D11DepthStencilState* LoadDSTestAndWrite(ID3D11Device* device);
static ID3D11DepthStencilState* LoadDSTestNoWrite(ID3D11Device* device);

void ResourceManager::initializeResources() {
    // Create my samplers
    shadowmap_sampler = LoadShadowMapSampler();
    mesh_sampler = LoadMeshTextureSampler();

    // Create my depth stencil states
    ds_test_and_write = LoadDSTestAndWrite(device);
    ds_test_no_write = LoadDSTestNoWrite(device);

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

ID3D11SamplerState* ResourceManager::getShadowMapSampler() {
    return shadowmap_sampler;
}

ID3D11SamplerState* ResourceManager::getMeshSampler() { return mesh_sampler; }

ID3D11DepthStencilState* ResourceManager::DSState_TestNoWrite() {
    return ds_test_no_write;
}
ID3D11DepthStencilState* ResourceManager::DSState_TestAndWrite() {
    return ds_test_and_write;
}

// LoadAssetFromGLTF:
// Uses the GLTFFile interface to load an asset from a GLTF file
bool ResourceManager::LoadAssetFromGLTF(const std::string& asset_name,
                                        const std::string& path,
                                        AtlasBuilder& tex_builder) {
    MeshBuilder mesh_builder = MeshBuilder();
    GLTFFile gltf_file = GLTFFile(path);
    Asset* asset = gltf_file.readFromFile(mesh_builder, tex_builder, device);

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
    MeshBuilder builder = MeshBuilder(BUILDER_POSITION);
    builder.addCube(Vector3(0, 0, 0), Quaternion(), 1.f);

    Asset* cube = new Asset();
    cube->addMesh(builder.generateMesh(device));

    return registerAsset("Cube", cube);
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

ID3D11DepthStencilState* LoadDSTestAndWrite(ID3D11Device* device) {
    D3D11_DEPTH_STENCIL_DESC desc = {};
    // Enable depth testing
    desc.DepthEnable = TRUE;
    // Standard depth test
    desc.DepthFunc = D3D11_COMPARISON_LESS;
    // Enable depth writing
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    // No stencil testing
    desc.StencilEnable = FALSE;

    ID3D11DepthStencilState* state = nullptr;
    HRESULT result = device->CreateDepthStencilState(&desc, &state);
    assert(SUCCEEDED(result));

    return state;
}

ID3D11DepthStencilState* LoadDSTestNoWrite(ID3D11Device* device) {
    D3D11_DEPTH_STENCIL_DESC desc = {};
    // Enable depth testing
    desc.DepthEnable = TRUE;
    // Standard depth test
    desc.DepthFunc = D3D11_COMPARISON_LESS;
    // Disable depth writing
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    // No stencil testing
    desc.StencilEnable = FALSE;

    ID3D11DepthStencilState* state = nullptr;
    HRESULT result = device->CreateDepthStencilState(&desc, &state);
    assert(SUCCEEDED(result));

    return state;
}

} // namespace Graphics
} // namespace Engine