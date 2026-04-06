#pragma once

#include <string>

#include "../Bindable.h"
#include "../Object.h"

#include "core/DataFilePath.h"

namespace Engine {
namespace Datamodel {
// DMMesh Class:
// Represents a mesh in the scene.
class DMMesh : public Object {
  private:
    DMTrackedProperty<std::string> mesh_name;
    DMTrackedProperty<std::string> colormap_name;

  public:
    DMMesh();
    ~DMMesh();

    void setMeshFile(const std::string& mesh_name);
    const std::string& getMeshFile();

    void setColorMapFile(const std::string& colormap_file);
    const std::string& getColorMapFile();
};

} // namespace Datamodel
} // namespace Engine