#pragma once

#include <memory>
#include <string>
#include <variant>

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

struct TextureRequestParams {
    // Texture Target Settings:
    // 1) TargetExisting: A pre-existing texture can be provided. That texture's settings are used as the config
    // 2) TargetNew: A new texture can be created. A config can be specified.
    struct TargetExisting {
        std::shared_ptr<Texture> target = nullptr;
    };
    struct TargetNew {
        std::string debugName = "";

        TextureLayout layout = TextureLayout::R8G8B8A8_UNORM;
        bool editable = false;
        uint8_t mipLevels = 1;
    };
    std::variant<TargetExisting, TargetNew> targetSettings;

    // Texture Data Settings:
    // 1) File IO: Read data from a file specified by a path
    // 2) TextureBuilder: Read data from a texture builder
    struct DataFromFile {
        std::string path;
    };
    struct DataFromBuilder {
        const TextureBuilder* builder;
    };
    std::variant<DataFromFile, DataFromBuilder> dataSettings;

    TextureRequestParams() = default;

    TargetNew& targetUseNew() { return targetSettings.emplace<TargetNew>(); }
    TargetExisting& targetUseExisting() {
        return targetSettings.emplace<TargetExisting>();
    }

    DataFromFile& dataFromFile() {
        return dataSettings.emplace<DataFromFile>();
    }
    DataFromBuilder& dataFromBuilder() {
        return dataSettings.emplace<DataFromBuilder>();
    }
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