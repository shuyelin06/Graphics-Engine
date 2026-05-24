#pragma once

#include <memory>
#include <string>
#include <variant>

#include "math/Matrix4.h"

#include "rendering/core/Camera.h"

namespace Engine {
namespace Graphics {
class VisualSystem;
class SceneManagerImpl;

struct RenderableMeshUpdatePacket {
    enum class Property {
        LocalMatrix,
        MeshName,
        ColorMapName,
        Invalid,
    };
    Property type = Property::Invalid;

    std::variant<Matrix4, std::string> data;
};

class SceneManager {
  public:
    static std::unique_ptr<SceneManager> create(VisualSystem* visualSystem);
    ~SceneManager();

    // Datamodel Updates
    struct UpdatePacket {
        enum Operation { Create, Destroy, Update };

        uint32_t handle; // Object Handle
        Operation operation;
        std::variant<Camera::UpdatePacket, RenderableMeshUpdatePacket> data;
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