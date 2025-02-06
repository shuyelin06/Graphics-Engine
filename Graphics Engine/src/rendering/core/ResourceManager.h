#pragma once

#include <map>
#include <string>
#include <unordered_map>

#include "datamodel/SceneGraph.h"

#include "../Direct3D11.h"

#include "Asset.h"
#include "MeshBuilder.h"
#include "TextureBuilder.h"

namespace Engine {
using namespace Datamodel;
namespace Graphics {

enum TextureSlot {
    Test = 0,
    Test2 = 1,
    Perlin = 2,
    TerrainGrass = 3,
    CapybaraTex = 4,
    TextureCount
};
enum SamplerSlot { ShadowMap = 0, MeshTexture = 1, SamplerCount };

// ResourceManager Class:
// Manages assets for the engine. Provides methods
// to load assets, and prepare them for rendering.
class ResourceManager {
  private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    std::unordered_map<std::string, Asset*> assets;
    std::unordered_map<std::string, Texture*> textures;

    std::vector<ID3D11SamplerState*> samplers;

    Mesh* terrain_meshes[CHUNK_X_LIMIT][CHUNK_Z_LIMIT];

  public:
    ResourceManager(ID3D11Device* device, ID3D11DeviceContext* context);
    ~ResourceManager();

    // Initialize assets
    void initialize();

    // Get an asset data by name
    Asset* getAsset(const std::string& name);
    // Get a texture by name
    Texture* getTexture(const std::string& name);

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
    Asset* LoadAssetFromOBJ(const std::string& path, const std::string& objFile);

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