#pragma once

#include <map>
#include <string>
#include <unordered_map>

#include "datamodel/SceneGraph.h"

#include "../Direct3D11.h"

#include "../core/Asset.h"
#include "AssetBuilder.h"
#include "TextureBuilder.h"

namespace Engine {
using namespace Datamodel;
namespace Graphics {

// ResourceManager Class:
// Manages assets for the engine. Provides methods
// to load assets, and prepare them for rendering.
class ResourceManager {
  private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    std::unordered_map<std::string, uint16_t> asset_map;
    std::vector<Asset*> assets;

    ID3D11SamplerState* shadowmap_sampler;
    ID3D11SamplerState* mesh_sampler;

    TextureAtlas* color_atlas;

  public:
    ResourceManager(ID3D11Device* device, ID3D11DeviceContext* context);
    ~ResourceManager();

    // Initialize assets
    void initialize();

    // Get builders
    MeshBuilder* createMeshBuilder();

    // Get an asset
    Asset* getAsset(const std::string& name);
    Asset* getAsset(uint16_t id);

    const Texture* getColorAtlas();

    // Get a sampler by name
    ID3D11SamplerState* getShadowMapSampler();
    ID3D11SamplerState* getMeshSampler();

  private:
    // Registers an asset by name, and returns it's ID
    uint16_t registerAsset(const std::string& name, Asset* asset);

    // Generate a cube
    Asset* LoadCube();

    // Load assets from files.
    Asset* LoadAssetFromOBJ(const std::string& path,
                            const std::string& objFile);

    bool LoadAssetFromGLTF(const std::string& asset_name,
                           const std::string& path,
                           AtlasBuilder& tex_builder);

    bool LoadTextureFromPNG(TextureBuilder& builder, std::string path,
                            std::string png_file);
    bool WriteTextureToPNG(ID3D11Texture2D* texture, std::string path,
                           std::string file);

    // Load Samplers
    ID3D11SamplerState* LoadShadowMapSampler();
    ID3D11SamplerState* LoadMeshTextureSampler();
};

} // namespace Graphics
} // namespace Engine