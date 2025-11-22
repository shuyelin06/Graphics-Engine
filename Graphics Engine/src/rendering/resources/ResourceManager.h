#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "../Direct3D11.h"

#include "../core/Asset.h"
#include "MeshBuilder.h"
#include "TextureBuilder.h"

namespace Engine {
namespace Graphics {

enum MeshPoolType {
    MeshPoolType_Terrain,
    MeshPoolType_Default,
    MeshPoolType_Count,
};

enum SystemMesh { SystemMesh_Cube };

// ResourceManager Class:
// Manages assets for the engine. Provides methods
// to load assets, and prepare them for rendering.
class ResourceManager {
  private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    std::unique_ptr<MeshPool> mesh_pools[MeshPoolType_Count];
    std::unique_ptr<TextureAtlas> color_atlas;

    std::vector<std::shared_ptr<Texture>> textures;
    std::vector<std::shared_ptr<Mesh>> meshes;

  public:
    ResourceManager(ID3D11Device* device, ID3D11DeviceContext* context);
    ~ResourceManager();

    // Initialize System Resources.
    // These are resources that exist for the entire application and are built
    // into the engine.
    void initializeSystemResources();

    // Get Resources
    std::shared_ptr<Mesh> getMesh(int index) const;

    const Texture* getColorAtlas();

    // Create Resources
    std::shared_ptr<Texture>
    LoadTextureFromFile(const std::string& relative_path);

    std::shared_ptr<Mesh> LoadMeshFromFile(const std::string& relative_path);

    std::shared_ptr<MeshBuilder> createMeshBuilder(MeshPoolType pool_type);
    MeshPool* getMeshPool(MeshPoolType pool_type);

  private:
    // System Asset Generation
    void LoadCubeMesh();

    // Load assets from files. TRY TO DEPRECATE
    void LoadAssetFromGLTF(const std::string& asset_name,
                           const std::string& path, AtlasBuilder& tex_builder);

    bool WriteTextureToPNG(ID3D11Texture2D* texture, std::string path,
                           std::string file);
};

} // namespace Graphics
} // namespace Engine