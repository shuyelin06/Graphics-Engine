#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <memory>

#include "../Direct3D11.h"

#include "../core/Asset.h"
#include "AssetBuilder.h"
#include "TextureBuilder.h"

namespace Engine {
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

    std::vector<std::shared_ptr<Texture>> textures;

    TextureAtlas* color_atlas;

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

  private:
    // Registers an asset by name, and returns it's ID
    uint16_t registerAsset(const std::string& name, Asset* asset);

    // Generate a cube
    bool LoadCube();

    // Load assets from files.
    bool LoadAssetFromGLTF(const std::string& asset_name,
                           const std::string& path, AtlasBuilder& tex_builder);

    bool WriteTextureToPNG(ID3D11Texture2D* texture, std::string path,
                           std::string file);

    Asset* LoadAssetFromFile(const std::string& path);
};

} // namespace Graphics
} // namespace Engine