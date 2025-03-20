#pragma once

#include <string>
#include <vector>

#include "MeshBuilder.h"

struct cgltf_accessor;

namespace Engine {
namespace Graphics {
// GLTFFile:
// Interface for reading GLTF Binary files.
class GLTFFile {
  private:
    std::string path;

  public:
    GLTFFile(const std::string& path);

    bool readFromFile(MeshBuilder& builder);

  private:
    void parseIndices(const cgltf_accessor* accessor, std::vector<MeshTriangle>& triangles);

    void parsePositions(const cgltf_accessor* accessor,
                        std::vector<MeshVertex>& vertex_data);
    void parseNormals(const cgltf_accessor* accessor,
                      std::vector<MeshVertex>& vertex_data);
    void parseTextureCoord(const cgltf_accessor* accessor,
                           std::vector<MeshVertex>& vertex_data);

    MeshVertex& createVertexAtIndex(int index,
                                    std::vector<MeshVertex>& vertex_data);
};

} // namespace Graphics
} // namespace Engine