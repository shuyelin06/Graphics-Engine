#pragma once

#include "datamodel/DMBinding.h"
#include "datamodel/objects/DMMesh.h"

#include "../resources/ResourceManager.h"
#include "Geometry.h"

namespace Engine {
using namespace Datamodel;

namespace Graphics {
// RenderableMesh:
// Interfaces with the DMMesh datamodel object.
// Stores the geometry information needed.
class RenderableMesh : public DMBinding {
  private:
    ResourceManager* resource_manager;
    
    std::shared_ptr<Geometry> geometry;
    std::string mesh_name;

    Matrix4 m_local_to_world;

    void pullDatamodelDataImpl(Object* obj) override;

  public:
    RenderableMesh(Object* dm_mesh, ResourceManager* resource_manager);
    ~RenderableMesh();

    std::weak_ptr<Geometry> getGeometry() const;
    bool isValidGeometry() const;

    const Matrix4& getLocalMatrix() const;
};

} // namespace Graphics
} // namespace Engine