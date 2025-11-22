#include "RenderableMesh.h"

namespace Engine {
namespace Graphics {
RenderableMesh::RenderableMesh(Object* dm_mesh, ResourceManager* resource_mgr)
    : DMBinding(dm_mesh) {
    resource_manager = resource_mgr;

    geometry = nullptr;
    mesh_name = "";
}
RenderableMesh::~RenderableMesh() = default;

// PullDatamodelImpl:
// Pulls the mesh name from the DMMesh, and attempts to kick off a load via
// resource manager.
void RenderableMesh::pullDatamodelDataImpl(Object* object) {
    DMMesh* dm_mesh = static_cast<DMMesh*>(object);

    if (mesh_name != dm_mesh->getMeshFile()) {
        mesh_name = dm_mesh->getMeshFile();
        std::shared_ptr<Material> material = std::make_shared<Material>(
            resource_manager->getTexture(SystemTexture_FallbackColormap));
        geometry = std::make_shared<Geometry>(resource_manager->LoadMeshFromFile(mesh_name),
                    std::move(material));
    }

    m_local_to_world = object->getLocalMatrix();
}

bool RenderableMesh::isValidGeometry() const { return geometry != nullptr; }
std::weak_ptr<Geometry> RenderableMesh::getGeometry() const { return geometry; }

const Matrix4& RenderableMesh::getLocalMatrix() const {
    return m_local_to_world;
}

} // namespace Graphics
} // namespace Engine