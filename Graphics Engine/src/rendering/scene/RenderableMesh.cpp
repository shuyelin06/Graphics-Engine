#include "RenderableMesh.h"
#include "RenderableMesh.h"

namespace Engine {
namespace Graphics {
RenderableMesh::RenderableMesh(ResourceManager* resourceManager)
    : mResourceManager(resourceManager) {
    mWorldFromLocal = Matrix4::Identity();
}
RenderableMesh::~RenderableMesh() = default;

void RenderableMesh::update(const UpdatePacket& packet) {
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

const Matrix4& RenderableMesh::getLocalMatrix() const {
    return mWorldFromLocal;
}

} // namespace Graphics
} // namespace Engine