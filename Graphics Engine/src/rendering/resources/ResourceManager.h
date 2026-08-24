#pragma once

#include <memory>
#include <string>
#include <variant>

#include "MeshBuilder.h"
#include "TextureBuilder.h"
#include "rendering/core/Mesh.h"
#include "rendering/core/Texture.h"

#include "rendering/core/Device.h"

namespace Engine
{
namespace Graphics
{
class ResourceManagerImpl;

// ResourceManager Class:
// Manages assets for the engine. Provides methods
// to load assets, and prepare them for rendering.
class ResourceManager
{
  public:
    static std::unique_ptr<ResourceManager> create(Device* device,
                                                   DeviceContext* context);
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
    std::shared_ptr<Geometry> getCubeMesh() const;

    // Create Resources
    std::shared_ptr<Geometry> LoadMeshFromFile(const std::string& relative_path);

    // Thread Safe Creation of Resources
    std::shared_ptr<Geometry> requestMesh(const MeshBuilder& mesh_builder);
    std::shared_ptr<Texture> requestTexture(const char* path);

    // Debug Display
    void imGui();

  private:
    std::unique_ptr<ResourceManagerImpl> mImpl;
    ResourceManager();
};

} // namespace Graphics
} // namespace Engine