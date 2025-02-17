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

#include "rendering/util/OBJFile.h"
#include "rendering/util/PNGFile.h"

#include "math/PerlinNoise.h"

using namespace std;

namespace Engine {
using namespace Math;

namespace Graphics {
VisualResourceManager::VisualResourceManager(ID3D11Device* _device,
                                             ID3D11DeviceContext* _context) {
    device = _device;
    context = _context;
}
VisualResourceManager::~VisualResourceManager() = default;

// Initialize:
// Loads assets into the asset manager.
void VisualResourceManager::initialize() {
    // Prepare my builders
    TextureBuilder::device = device;

    // Create my textures
    TextureBuilder tex_builder = TextureBuilder(10, 10);

    // textures[Test] = tex_builder.generate();

    //// Noise
    // tex_builder.reset(1000, 1000);
    // for (int i = 1; i <= 1000; i++) {
    //     for (int j = 1; j <= 1000; j++) {
    //         float val =
    //             PerlinNoise::noise2D(i * 4.f / 1087.f, j * 4.f / 1087.f);
    //         assert(0 <= val && val <= 1);
    //         unsigned char convert = (int)(255 * val);
    //         tex_builder.setColor(i - 1, j - 1,
    //                              {convert, convert, convert, 255});
    //     }
    // }
    // textures[Perlin] = tex_builder.generate();
    // WriteTextureToPNG(textures[Perlin]->texture, "data/", "perlin.png");

    // LoadTextureFromPNG(tex_builder, "data/", "test.png");
    //  textures[Test2] = tex_builder.generate();

    LoadTextureFromPNG(tex_builder, "data/", "grass.png");
    textures["TerrainGrass"] = tex_builder.generate();

    LoadTextureFromPNG(tex_builder, "data/", "Capybara_BaseColor.png");
    textures["CapybaraTex"] = tex_builder.generate();

    // Create my samplers
    shadowmap_sampler = LoadShadowMapSampler();
    mesh_sampler = LoadMeshTextureSampler();

    // Create my assets
    MeshBuilder mesh_builder = MeshBuilder(device);

    assets["Cube"] = LoadCube(mesh_builder);
    // Fox by Jake Blakeley [CC-BY] via Poly Pizza
    assets["Fox"] = LoadAssetFromOBJ("data/", "model.obj");

    assets["Capybara"] = LoadAssetFromOBJ("data/", "Capybara.obj");
    // Capybara
    // Capybara by Poly by Google [CC-BY] via Poly Pizza
}

// Get Resources:
// Return resources by name.
Asset* VisualResourceManager::getAsset(const std::string& name) {
    if (assets.contains(name))
        return assets[name];
    else
        return nullptr;
}

Texture* VisualResourceManager::getTexture(const std::string& name) {
    if (textures.contains(name))
        return textures[name];
    else
        return nullptr;
}

ID3D11SamplerState* VisualResourceManager::getShadowMapSampler() {
    return shadowmap_sampler;
}

ID3D11SamplerState* VisualResourceManager::getMeshSampler() {
    return mesh_sampler;
}

// LoadTextureFromPNG:
// Uses the PNGFile interface to load a texture from a PNG file
bool VisualResourceManager::LoadTextureFromPNG(TextureBuilder& builder,
                                               std::string path,
                                               std::string file) {
    PNGFile png_file = PNGFile(path + file);
    return png_file.readTextureFromFile(builder);
}

// WriteTextureToPNG:
// Uses the PNGFile interface to write a texture to a PNG file
bool VisualResourceManager::WriteTextureToPNG(ID3D11Texture2D* texture,
                                              std::string path,
                                              std::string file) {
    PNGFile png_file = PNGFile(path + file);
    return png_file.writeTextureToFile(device, context, texture);
}

// Helper parsing functions

Asset* VisualResourceManager::LoadAssetFromOBJ(const std::string& path,
                                               const std::string& objFile) {
    MeshBuilder mesh_builder = MeshBuilder(device);
    TextureBuilder texture_builder(0, 0);

    OBJFile obj_file = OBJFile(path, objFile);
    return obj_file.readAssetFromFile(mesh_builder, texture_builder);
}

// Hard-Coded Cube Creator
// Used in debugging
Asset* VisualResourceManager::LoadCube(MeshBuilder& builder) {
    builder.reset();
    builder.addCube(Vector3(0, 0, 0), 1.f);

    Asset* cube = new Asset();
    cube->addMesh(builder.generate());

    return cube;
}

// Load___Sampler:
// Create Texture Samplers!
ID3D11SamplerState* VisualResourceManager::LoadShadowMapSampler() {
    ID3D11SamplerState* sampler;

    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
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

ID3D11SamplerState* VisualResourceManager::LoadMeshTextureSampler() {
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