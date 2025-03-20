#pragma once

#include <string>
#include <vector>

#include "MeshBuilder.h"

struct cgltf_accessor;
struct cgltf_material;

namespace Engine {
namespace Graphics {
// GLTFFile:
// Interface for reading GLTF Binary files.
class GLTFFile {
  private:
    std::string path;

  public:
    GLTFFile(const std::string& path);

    Asset* readFromFile(MeshBuilder& builder);

  private:
    // Index Buffer Parsing
    void parseIndices(const cgltf_accessor* accessor,
                      std::vector<MeshTriangle>& triangles);

    // Vertex Buffer Parsing
    void parsePositions(const cgltf_accessor* accessor,
                        std::vector<MeshVertex>& vertex_data);
    void parseNormals(const cgltf_accessor* accessor,
                      std::vector<MeshVertex>& vertex_data);
    void parseTextureCoord(const cgltf_accessor* accessor,
                           std::vector<MeshVertex>& vertex_data);
    
    MeshVertex& createVertexAtIndex(int index,
                                    std::vector<MeshVertex>& vertex_data);

    // Material Parsing
    void parseMaterial(const cgltf_material* mat_data, Material& material);
    
};

} // namespace Graphics
} // namespace Engine