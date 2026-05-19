#pragma once

#include "datamodel/DMBinding.h"
#include "datamodel/objects/DMMesh.h"

#include "rendering/resources/MaterialManager.h"
#include "rendering/resources/ResourceManager.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
// DefaultMesh:
// Interfaces with the DMMesh datamodel object.
// Stores the geometry information needed.
class DefaultMesh {
  private:
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material_new;

    Material_DEPRECATED material;

    Matrix4 mWorldFromLocal;

  public:
    DefaultMesh();
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
    void update(const UpdatePacket& packet, ResourceManager* resourceManager,
                bool& dirty);

    std::shared_ptr<Mesh> getMesh() const;
    std::shared_ptr<Material> getMaterialNew() const;

    Material_DEPRECATED getMaterial() const;

    const Matrix4& getLocalMatrix() const;

    bool isReady() const;
};

} // namespace Graphics
} // namespace Engine