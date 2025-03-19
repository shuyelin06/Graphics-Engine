#pragma once

#include <string>

#include "../Direct3D11.h"
#include "../core/Asset.h"

#include "MeshBuilder.h"
#include "TextureBuilder.h"

namespace Engine {
namespace Graphics {
// OBJFile Class:
// Class that provides an interface for working with
// OBJ files. Allows reading to and writing from these files.
// WIP TODO: Move OBJ file parsing to this class
class OBJFile {
  private:
    std::string path;
    std::string file_name;

  public:
    OBJFile(const std::string& path, const std::string& file_name);

    Asset* readAssetFromFile(MeshBuilder& mesh_builder,
                             TextureBuilder& tex_builder);
};

} // namespace Graphics
} // namespace Engine