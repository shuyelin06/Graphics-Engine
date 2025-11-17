#include "RenderableMesh.h"

namespace Engine {
namespace Graphics {
RenderableMesh::RenderableMesh(Object* dm_mesh, ResourceManager* resource_mgr)
    : DMBinding(dm_mesh) {
    resource_manager = resource_mgr;

    mesh = nullptr;
    mesh_name = "";
}
RenderableMesh::~RenderableMesh() = default;

// PullDatamodelImpl:
// Pulls the mesh name from the DMMesh, and attempts to kick off a load via
// resource manager.
void RenderableMesh::pullDatamodelDataImpl(Object* object) {
    DMMesh* dm_mesh = static_cast<DMMesh*>(object);

    if (mesh_name != dm_mesh->getMeshName()) {
        mesh_name = dm_mesh->getMeshName();
        mesh = resource_manager->LoadMeshFromFile(mesh_name);
    }
}

bool RenderableMesh::isValidMesh() const { return mesh != nullptr; }
std::shared_ptr<Mesh> RenderableMesh::getMesh() const { return mesh; }

} // namespace Graphics
} // namespace Engine