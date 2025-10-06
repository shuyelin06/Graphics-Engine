#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "../Direct3D11.h"

#include "../core/Asset.h"
#include "AssetBuilder.h"
#include "TextureBuilder.h"

namespace Engine {
namespace Graphics {

enum MeshPoolType {
    MeshPoolType_Default,
    MeshPoolType_Count,
};
// ResourceManager Class:
// Manages assets for the engine. Provides methods
// to load assets, and prepare them for rendering.
class ResourceManager {
  private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    std::unique_ptr<MeshPool> mesh_pools[MeshPoolType_Count];
    std::unique_ptr<TextureAtlas> color_atlas;
    std::vector<std::unique_ptr<Asset>> assets;

    std::unordered_map<std::string, uint16_t> asset_map;

    std::vector<std::shared_ptr<Texture>> textures;

  public:
    ResourceManager(ID3D11Device* device, ID3D11DeviceContext* context);
    ~ResourceManager();

    // Initialize assets
    void initializeResources();

    // Get an asset
    Asset* getAsset(const std::string& name);
    Asset* getAsset(uint16_t id);

    const Texture* getColorAtlas();

    // Create Resources
    std::shared_ptr<Texture>
    LoadTextureFromFile(const std::string& relative_path);

    std::shared_ptr<MeshBuilder> createMeshBuilder(MeshPoolType pool_type);
    
  private:
    // Asset Generation
    uint16_t registerAsset(const std::string& name,
                           std::unique_ptr<Asset>& asset);
    void LoadCubeAsset();

    // Load assets from files.
    void LoadAssetFromGLTF(const std::string& asset_name,
                           const std::string& path, AtlasBuilder& tex_builder);

    bool WriteTextureToPNG(ID3D11Texture2D* texture, std::string path,
                           std::string file);
};

} // namespace Graphics
} // namespace Engine