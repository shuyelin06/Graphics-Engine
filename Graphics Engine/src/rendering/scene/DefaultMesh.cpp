#include "DefaultMesh.h"

namespace Engine {
namespace Graphics {
DefaultMesh::DefaultMesh(ResourceManager* resourceManager)
    : mResourceManager(resourceManager) {
    mWorldFromLocal = Matrix4::Identity();
}
DefaultMesh::~DefaultMesh() = default;

void DefaultMesh::update(const UpdatePacket& packet) {
    GeometryDesc desc;
    bool dirty = false;

    switch (packet.type) {
    case UpdatePacket::Property::LocalMatrix:
        mWorldFromLocal = std::get<Matrix4>(packet.data);
        break;

    case UpdatePacket::Property::MeshName: {
        const std::string& meshName = std::get<std::string>(packet.data);
        mesh = mResourceManager->LoadMeshFromFile(meshName);
    } break;

    case UpdatePacket::Property::ColorMapName: {
        const std::string& colormapName = std::get<std::string>(packet.data);
        material.colormap = mResourceManager->LoadTextureFromFile(colormapName);
    } break;
    }
}

std::shared_ptr<Mesh> DefaultMesh::getMesh() const { return mesh; }
Material DefaultMesh::getMaterial() const { return material; }

const Matrix4& DefaultMesh::getLocalMatrix() const {
    return mWorldFromLocal;
}

} // namespace Graphics
} // namespace Engine