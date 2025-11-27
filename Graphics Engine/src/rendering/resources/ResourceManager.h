#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "../Direct3D11.h"

#include "MeshBuilder.h"
#include "TextureBuilder.h"
#include "rendering/core/Geometry.h"
#include "rendering/core/Material.h"
#include "rendering/core/Mesh.h"
#include "rendering/core/Texture.h"

namespace Engine {
namespace Graphics {

enum MeshPoolType {
    MeshPoolType_Terrain,
    MeshPoolType_Default,
    MeshPoolType_Count,
};

enum SystemMesh { SystemMesh_Cube = 0 };
enum SystemTexture { SystemTexture_FallbackColormap = 0 };

struct GeometryDesc {
    std::shared_ptr<Mesh> mesh;
    Material material;
};

// ResourceManager Class:
// Manages assets for the engine. Provides methods
// to load assets, and prepare them for rendering.
class ResourceManager {
  private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    std::unique_ptr<MeshPool> mesh_pools[MeshPoolType_Count];
    std::vector<std::shared_ptr<Mesh>> meshes;

    std::vector<std::shared_ptr<Texture>> textures;

    std::vector<std::shared_ptr<Geometry>> geometries;

  public:
    ResourceManager(ID3D11Device* device, ID3D11DeviceContext* context);
    ~ResourceManager();

    // Initialize System Resources.
    // These are resources that exist for the entire application and are built
    // into the engine.
    void initializeSystemResources();

    // Get Resources
    std::shared_ptr<Mesh> getMesh(int index) const;
    std::shared_ptr<Texture> getTexture(int index) const;
    std::shared_ptr<Geometry> getGeometry(int index) const;

    // Create Resources
    std::shared_ptr<Geometry> CreateGeometry(const GeometryDesc& desc);
    std::shared_ptr<Texture>
    LoadTextureFromFile(const std::string& relative_path);
    std::shared_ptr<Mesh> LoadMeshFromFile(const std::string& relative_path);

    std::shared_ptr<MeshBuilder> createMeshBuilder(MeshPoolType pool_type);
    std::shared_ptr<TextureBuilder> createTextureBuilder();

    MeshPool* getMeshPool(MeshPoolType pool_type);

    // Debug Display
    void imGui();

  private:
    // System Asset Generation
    void LoadCubeMesh();

    void LoadFallbackColormap();

    bool WriteTextureToPNG(ID3D11Texture2D* texture, std::string path,
                           std::string file);
};

} // namespace Graphics
} // namespace Engine