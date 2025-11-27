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

    GeometryDesc desc;
    bool dirty = false;

    if (mesh_name != dm_mesh->getMeshFile()) {
        dirty = true;

        mesh_name = dm_mesh->getMeshFile();
        desc.mesh = resource_manager->LoadMeshFromFile(mesh_name);
    }

    if (colormap_name != dm_mesh->getColorMapFile())
    {
        dirty = true;

        colormap_name = dm_mesh->getColorMapFile();
        desc.material.colormap = resource_manager->LoadTextureFromFile(colormap_name);
    }

    if (dirty)
    {
        geometry = resource_manager->CreateGeometry(desc);
    }

    m_local_to_world = object->getLocalMatrix();
}

bool RenderableMesh::isValidGeometry() const { return geometry != nullptr; }
std::shared_ptr<Geometry> RenderableMesh::getGeometry() const { return geometry; }

const Matrix4& RenderableMesh::getLocalMatrix() const {
    return m_local_to_world;
}

} // namespace Graphics
} // namespace Engine