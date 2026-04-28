#pragma once

#include <memory>
#include <variant>

#include "rendering/core/Camera.h"
#include "rendering/scene/DefaultMesh.h"

namespace Engine {
namespace Graphics {
class VisualSystem;
class SceneManagerImpl;

class SceneManager {
  public:
    static std::unique_ptr<SceneManager> create(VisualSystem* visualSystem);
    ~SceneManager();

    // Datamodel Updates
    struct UpdatePacket {
        enum Operation { Create, Destroy, Update };

        uint32_t handle; // Object Handle
        Operation operation;
        std::variant<Camera::UpdatePacket, DefaultMesh::UpdatePacket> data;
    };
    void submitUpdatePacket(const UpdatePacket& packet);

    // Main Update Loop
    void update();

    // Accessors
    Camera* getMainCamera();

  private:
    std::unique_ptr<SceneManagerImpl> mImpl;
    SceneManager();
};

} // namespace Graphics
} // namespace Engine