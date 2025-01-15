#pragma once

#include <map>
#include <string>

#include "Direct3D11.h"

#include "utility/IndexMap.h"
#include "datamodel/SceneGraph.h"

#include "rendering/AssetIDs.h"
#include "rendering/core/Asset.h"
#include "rendering/core/AssetBuilder.h"

namespace Engine {
namespace Graphics {
using namespace Utility;
using namespace Datamodel;

// enum AssetSlot from AssetIDs.h

enum TextureSlot { Test = 0, Test2 = 1, Perlin = 2, TerrainGrass = 3, TextureCount };
enum SamplerSlot { ShadowMap = 0, MeshTexture = 1, SamplerCount };

// AssetManager Class:
// Manages assets for the engine. Provides methods
// to load assets, and prepare them for rendering.
class AssetManager {
  private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    std::vector<Asset*> assets;
    std::vector<Texture*> textures;
    std::vector<ID3D11SamplerState*> samplers;

    Mesh* terrain_meshes[CHUNK_X_LIMIT][CHUNK_Z_LIMIT];

  public:
    AssetManager(ID3D11Device* device, ID3D11DeviceContext* context);
    ~AssetManager();

    // Initialize assets
    void initialize();

    // Get an asset data by name
    Asset* getAsset(AssetSlot asset);
    // Get a texture by name
    Texture* getTexture(TextureSlot texture);
    // Get a sampler by name
    ID3D11SamplerState* getSampler(SamplerSlot sampler);

    // Get a terrain mesh by its positional ID. Caches the generated terrain so
    // we don't have to generate it again.
    Mesh* getTerrainMesh(int x, int z, TerrainData data);

  private:
    // Generate a cube
    Asset* LoadCube(MeshBuilder& builder);

    // Generate terrain given terrain chunk data
    Mesh* GenerateTerrainMesh(MeshBuilder& builder, TerrainData data);

    // Load assets from files.
    Asset* LoadAssetFromOBJ(MeshBuilder& builder, std::string path,
                            std::string objFile, std::string assetName);

    bool LoadTextureFromPNG(TextureBuilder& builder, std::string path,
                            std::string pngFile);

    // Load Samplers
    ID3D11SamplerState* LoadShadowMapSampler();
    ID3D11SamplerState* LoadMeshTextureSampler();

    // Write assets to files
    bool WriteTextureToPNG(ID3D11Texture2D* texture, std::string file_name);
};

} // namespace Graphics
} // namespace Engine