#include "DMMesh.h"

namespace Engine {
namespace Datamodel {
DMMesh::DMMesh() : Object(), Bindable<DMMesh>(this) {
    mesh_name = "";
    DMMesh::SignalObjectCreation(this);
}
DMMesh::~DMMesh() = default;

void DMMesh::setMeshName(const std::string& name) { mesh_name = name; }
const std::string& DMMesh::getMeshName() { return mesh_name; }

} // namespace Datamodel
} // namespace Engine