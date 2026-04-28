#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "MeshBuilder.h"
#include "TextureBuilder.h"
#include "rendering/core/Material.h"
#include "rendering/core/Mesh.h"
#include "rendering/core/Texture.h"

#include "rendering/pipeline/ConstantBuffer.h"

#include "rendering/pipeline/techniques/VSTerrain.h"

// Forward Declare so that systems using ResourceManager don't pull in the D3D11
// implementation
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace Engine {
namespace Graphics {
class ResourceManagerImpl;

enum MeshPoolType {
    MeshPoolType_Terrain,
    MeshPoolType_Default,
    MeshPoolType_Count,
};

// - SystemMesh Cube: Unit cube from (-0.5, -0.5, -0.5) to (0.5, 0.5, 0.5)
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
  public:
    static std::unique_ptr<ResourceManager>
    create(ID3D11Device* device, ID3D11DeviceContext* context);
    ~ResourceManager();

    // Initialize System Resources.
    // These are resources that exist for the entire application and are built
    // into the engine.
    void initializeSystemResources();

    // Update Loop.
    // Serve the various requests received by the resource manager.
    void updatePerform();

    // Get Resources
    std::shared_ptr<Mesh> getMesh(int index) const;
    std::shared_ptr<Texture> getTexture(int index) const;

    // Create Resources
    std::shared_ptr<Texture>
    LoadTextureFromFile(const std::string& relative_path);
    std::shared_ptr<Mesh> LoadMeshFromFile(const std::string& relative_path);

    std::shared_ptr<TextureBuilder> createTextureBuilder();

    MeshPool* getMeshPool(MeshPoolType pool_type);

    std::shared_ptr<Mesh> requestMesh(const MeshBuilder& mesh_builder);
    std::shared_ptr<VSTerrain> requestTerrainMesh();

    // Debug Display
    void imGui();

  private:
    std::unique_ptr<ResourceManagerImpl> mImpl;
    ResourceManager();
};

} // namespace Graphics
} // namespace Engine