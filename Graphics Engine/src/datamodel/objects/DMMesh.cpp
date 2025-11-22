#include "DMMesh.h"

namespace Engine {
namespace Datamodel {
DMMesh::DMMesh() : Object(), Bindable<DMMesh>(this) {
    mesh_name = "";
    DMMesh::SignalObjectCreation(this);
}
DMMesh::~DMMesh() = default;

void DMMesh::setMeshFile(const std::string& name) { mesh_name = name; }
const std::string& DMMesh::getMeshFile() { return mesh_name; }

void DMMesh::setColorMapFile(const std::string& colormap_file) {
    colormap_name = colormap_file;
}
const std::string& DMMesh::getColorMapFile() { return colormap_name; }

} // namespace Datamodel
} // namespace Engine