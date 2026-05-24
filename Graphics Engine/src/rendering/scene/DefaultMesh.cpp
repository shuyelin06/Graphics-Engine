#include "DefaultMesh.h"

namespace Engine {
namespace Graphics {
DefaultMesh::DefaultMesh() { mWorldFromLocal = Matrix4::Identity(); }
DefaultMesh::~DefaultMesh() = default;

void DefaultMesh::update(const UpdatePacket& packet,
                         ResourceManager* resourceManager, bool& dirty) {
    switch (packet.type) {
    case UpdatePacket::Property::LocalMatrix:
        mWorldFromLocal = std::get<Matrix4>(packet.data);
        break;

    case UpdatePacket::Property::MeshName: {
        const std::string& meshName = std::get<std::string>(packet.data);
        mesh = resourceManager->LoadMeshFromFile(meshName);

        dirty = true;
    } break;

    case UpdatePacket::Property::ColorMapName: {
        const std::string& colormapName = std::get<std::string>(packet.data);
        // material.colormap = resourceManager->LoadTextureFromFile(colormapName);

        dirty = true;
    } break;
    }
}

std::shared_ptr<Mesh> DefaultMesh::getMesh() const { return mesh; }
std::shared_ptr<Material> DefaultMesh::getMaterialNew() const {
    return material;
}

const Matrix4& DefaultMesh::getLocalMatrix() const { return mWorldFromLocal; }

bool DefaultMesh::isReady() const {
    return mesh->ready && material->ready();
}

} // namespace Graphics
} // namespace Engine