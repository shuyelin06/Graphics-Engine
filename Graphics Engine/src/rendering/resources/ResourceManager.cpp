#include "ResourceManager.h"

#include <fstream>
#include <iostream>

#include <map>
#include <regex>
#include <string.h>
#include <vector>

#include <assert.h>

#include "math/Vector2.h"
#include "math/Vector3.h"

#include "files/FileReader.h"

// We use the lodepng library to read PNG files.
// See https://github.com/lvandeve/lodepng
#include "lodepng/lodepng.h"

#include "files/GLTFFile.h"
#include "files/PNGFile.h"

using namespace std;

namespace Engine {
using namespace Math;

namespace Graphics {
static const std::string RESOURCE_FOLDER = "data/";

ResourceManager::ResourceManager(ID3D11Device* _device,
                                 ID3D11DeviceContext* _context) {
    device = _device;
    context = _context;
}
ResourceManager::~ResourceManager() = default;

// Initialize:
// Loads assets into the asset manager.
void ResourceManager::initializeResources() {
    //  Empirical testing has shown that
    // 300,000 vertices, 200,000 indices is enough
    mesh_pools[MeshPoolType_Terrain] = std::make_unique<MeshPool>(
        (1 << POSITION) | (1 << NORMAL), 200000, 300000);
    mesh_pools[MeshPoolType_Default] =
        std::make_unique<MeshPool>(0xFFFF, 100000, 100000);

    // Stores an atlas of material colors to avoid the need for rebinds later
    AtlasBuilder atlas_builder = AtlasBuilder(4096, 4096);

    // --- Load Assets Here ---
    LoadCubeAsset();

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

    color_atlas = std::unique_ptr<TextureAtlas>(atlas_builder.generate(device));

    mesh_pools[MeshPoolType_Default]->createGPUResources(device);
    mesh_pools[MeshPoolType_Default]->updateGPUResources(context);
}

// RegisterAsset:
// Registers an asset by name, and returns it's ID
uint16_t ResourceManager::registerAsset(const std::string& name,
                                        std::unique_ptr<Asset>& asset) {
    const uint16_t id = (uint16_t)assets.size();

    asset_map[name] = id;
    assets.emplace_back(std::move(asset));

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

Asset* ResourceManager::getAsset(uint16_t id) { return assets[id].get(); }

const Texture* ResourceManager::getColorAtlas() {
    return color_atlas->getTexture();
}

// LoadAssetFromGLTF:
// Uses the GLTFFile interface to load an asset from a GLTF file
void ResourceManager::LoadAssetFromGLTF(const std::string& asset_name,
                                        const std::string& path,
                                        AtlasBuilder& tex_builder) {
    std::shared_ptr<MeshBuilder> mesh_builder =
        createMeshBuilder(MeshPoolType_Default);
    GLTFFile gltf_file = GLTFFile(path);
    std::unique_ptr<Asset> asset = std::unique_ptr<Asset>(
        gltf_file.readFromFile(*mesh_builder, tex_builder, device, context));

    if (asset != nullptr)
        registerAsset(asset_name, asset);
    else
        assert(false);
}

// LoadTexture:
// Code path for loading all textures.
std::shared_ptr<Texture>
ResourceManager::LoadTextureFromFile(const std::string& relative_path) {
    const std::string full_path = RESOURCE_FOLDER + relative_path;

    // Matches to find the file name and extension separately.
    // (?:.+/)* matches the path but does not put it in a capture group.
    std::regex name_pattern("(?:.+/)*([a-zA-Z]+)\\.([a-zA-Z]+)");
    smatch match;
    regex_search(relative_path, match, name_pattern);

    Texture* output = nullptr;

    if (match.size() == 3) {
        // If name is ever needed:
        // const std::string name = match[1];
        const std::string extension = match[2];

        FileReader reader = FileReader(full_path);
        if (reader.readFileData()) {
            TextureBuilder builder = TextureBuilder(0, 0);

            if (extension == "png") {
                PNGFile::ReadPNGData(reader.getData(), builder);
                output = builder.generate(device);
            } else
                assert(false); // Unsupported Format
        }
    }

    textures.emplace_back(std::shared_ptr<Texture>(output));
    return textures.back();
}

std::shared_ptr<Mesh>
ResourceManager::LoadMeshFromFile(const std::string& relative_path) {
    const std::string full_path = RESOURCE_FOLDER + relative_path;

    // Matches to find the file name and extension separately.
    // (?:.+/)* matches the path but does not put it in a capture group.
    std::regex name_pattern("(?:.+/)*([a-zA-Z]+)\\.([a-zA-Z]+)");
    smatch match;
    regex_search(relative_path, match, name_pattern);

    std::shared_ptr<Mesh> output = nullptr;

    if (match.size() == 3) {
        // If name is ever needed:
        // const std::string name = match[1];
        const std::string extension = match[2];

        FileReader reader = FileReader(full_path);
        if (reader.readFileData()) {
            std::shared_ptr<MeshBuilder> builder =
                createMeshBuilder(MeshPoolType_Default);

            if (extension == "glb") {
                GLTFFile::ReadGLTFData(full_path, *builder);
                output = builder->generateMesh(context);
            } else
                assert(false); // Unsupported Format
        }
    }

    if (output != nullptr) {
        meshes.emplace_back(std::move(output));
        return meshes.back();
    } else
        return nullptr;
}

std::shared_ptr<MeshBuilder>
ResourceManager::createMeshBuilder(MeshPoolType pool_type) {
    return std::make_shared<MeshBuilder>(mesh_pools[pool_type].get());
}

MeshPool* ResourceManager::getMeshPool(MeshPoolType pool_type) {
    return mesh_pools[pool_type].get();
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
void ResourceManager::LoadCubeAsset() {
    std::shared_ptr<MeshBuilder> builder =
        createMeshBuilder(MeshPoolType_Default);
    builder->addLayout(POSITION);
    builder->addCube(Vector3(0, 0, 0), Quaternion(), 1.f);
    std::shared_ptr<Mesh> mesh = builder->generateMesh(context);

    std::unique_ptr<Asset> cube = std::make_unique<Asset>();
    cube->addMesh(mesh);

    registerAsset("Cube", cube);
}

} // namespace Graphics
} // namespace Engine