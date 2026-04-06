#include "DMMesh.h"

namespace Engine {
namespace Datamodel {
DMMesh::DMMesh()
    : Object("Mesh"), mesh_name(&getDMHandle(), "MeshName"),
      colormap_name(&getDMHandle(), "ColormapName") {
    mesh_name.writeProperty("");
    colormap_name.writeProperty("");
}
DMMesh::~DMMesh() = default;

void DMMesh::setMeshFile(const std::string& name) {
    mesh_name.writeProperty(name);
}
const std::string& DMMesh::getMeshFile() { return mesh_name.readProperty(); }

void DMMesh::setColorMapFile(const std::string& colormap_file) {
    colormap_name.writeProperty(colormap_file);
}
const std::string& DMMesh::getColorMapFile() {
    return colormap_name.readProperty();
}

} // namespace Datamodel
} // namespace Engine