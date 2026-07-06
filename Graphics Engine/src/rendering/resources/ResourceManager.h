#pragma once

#include <memory>
#include <string>

#include "MeshBuilder.h"
#include "TextureBuilder.h"
#include "rendering/core/Mesh.h"
#include "rendering/core/Texture.h"

// Forward Declare so that systems using ResourceManager don't pull in the D3D11
// implementation
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;

namespace Engine {
namespace Graphics {
class ResourceManagerImpl;

enum MeshPoolType {
    MeshPoolType_Terrain,
    MeshPoolType_Default,
    MeshPoolType_Count,
};

enum class TextureRequestType {
    CreateFromFile = 0,
    CreateFromBuilder = 1,
    UpdateFromBuilder = 2,
};
struct TextureRequestParams {
    TextureRequestType requestType;
    std::string debugName;

    std::string path;              // CreateFromFile
    bool editable;                 // CreateFromFile, CreateFromBuilder
    const TextureBuilder* builder; // CreateFromBuilder, UpdateFromBuilder
    std::shared_ptr<Texture> target; // UpdateFromBuilder

    TextureRequestParams(const std::string& debugName = "");
    void initCreateFromFile(const std::string& path, bool editable);
    void initCreateFromBuilder(const TextureBuilder& builder, bool editable);
    void initUpdateFromBuilder(const TextureBuilder& builder,
                               const std::shared_ptr<Texture>& target);
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

    // System Resources
    std::shared_ptr<Texture> getFallbackColormap() const;

    // Create Resources
    std::shared_ptr<Mesh> LoadMeshFromFile(const std::string& relative_path);

    // Thread Safe Creation of Resources
    std::shared_ptr<Mesh> requestMesh(const MeshBuilder& mesh_builder);
    std::shared_ptr<Texture> requestTexture(const TextureRequestParams& params);

    // Not-Thread Safe Creation / Modification of Resources.
    // Must be done on main thread if called.
    void clearDepthStencil(const Texture& texture);

    // Debug Display
    void imGui();

  private:
    std::unique_ptr<ResourceManagerImpl> mImpl;
    ResourceManager();
};

} // namespace Graphics
} // namespace Engine