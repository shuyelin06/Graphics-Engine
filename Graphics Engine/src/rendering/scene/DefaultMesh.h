#pragma once

#include "datamodel/DMBinding.h"
#include "datamodel/objects/DMMesh.h"

#include "rendering/resources/ResourceManager.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
// DefaultMesh:
// Interfaces with the DMMesh datamodel object.
// Stores the geometry information needed.
class DefaultMesh {
  private:
    ResourceManager* mResourceManager;

    std::shared_ptr<Mesh> mesh;
    Material material;

    Matrix4 mWorldFromLocal;

  public:
    DefaultMesh(ResourceManager* resourceManager);
    ~DefaultMesh();

    // Updating
    struct UpdatePacket {
        enum class Property {
            LocalMatrix,
            MeshName,
            ColorMapName,
            Invalid,
        };
        Property type = Property::Invalid;

        std::variant<Matrix4, std::string> data;
    };
    void update(const UpdatePacket& packet);

    std::shared_ptr<Mesh> getMesh() const;
    Material getMaterial() const;

    const Matrix4& getLocalMatrix() const;
};

} // namespace Graphics
} // namespace Engine