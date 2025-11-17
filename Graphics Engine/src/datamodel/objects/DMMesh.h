#pragma once

#include <string>

#include "../Bindable.h"
#include "../Object.h"

namespace Engine {
namespace Datamodel {
// DMMesh Class:
// Represents a mesh in the scene.
class DMMesh : public Object, public Bindable<DMMesh> {
  private:
    std::string mesh_name;

  public:
    DMMesh();
    ~DMMesh();

    void setMeshName(const std::string& mesh_name);
    const std::string& getMeshName();
};

} // namespace Datamodel
} // namespace Engine