#pragma once

#include <map>
#include <string>

#include "Direct3D11.h"

#include "rendering/components/Asset.h"
#include "rendering/components/AssetBuilder.h"

namespace Engine {
namespace Graphics {
// Assets
enum AssetSlot {
    Cube = 0,
    Fox = 1,
    Terrain = 2, // Temp
    AssetCount
};

enum TextureSlot { 
    Test = 0, 
    Test2 = 1,
    Perlin = 2,
    TextureCount 
};

enum SamplerSlot { ShadowMap = 0, MeshTexture = 1, SamplerCount };

// AssetManager Class:
// Manages assets for the engine. Provides methods
// to load assets, and prepare them for rendering.
class AssetManager {
  private:
    ID3D11Device* device;

    std::vector<Asset*> assets;
    std::vector<Texture*> textures;
    std::vector<ID3D11SamplerState*> samplers;

  public:
    AssetManager(ID3D11Device* device);
    ~AssetManager();

    // Initialize assets
    void initialize();

    // Get an asset data by name
    Asset* getAsset(AssetSlot asset);
    // Get a texture by name
    Texture* getTexture(TextureSlot texture);
    // Get a sampler by name
    ID3D11SamplerState* getSampler(SamplerSlot sampler);

  private:
    // Generate a cube
    Asset* LoadCube(MeshBuilder& builder);

    // Load an asset from an OBJ file. Returns the index of the
    // asset in the manager on success.
    Asset* LoadAssetFromOBJ(MeshBuilder& builder, std::string path,
                            std::string objFile, std::string assetName);

    bool LoadTextureFromPNG(TextureBuilder& builder, std::string path,
                            std::string pngFile);

    // Load Samplers
    ID3D11SamplerState* LoadShadowMapSampler();
    ID3D11SamplerState* LoadMeshTextureSampler();
};

} // namespace Graphics
} // namespace Engine