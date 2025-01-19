#pragma once

#include <vector>

#include "Asset.h"

namespace Engine {
namespace Graphics {
// MeshBuilder Class:
// Enables creation of meshes. Meshes are represented by a vertex and index
// buffer. The vertex buffer stores all vertices in the mesh, and the index
// buffer references these vertices by index to create triangles for the mesh.
struct MeshVertex {
    Vector3 position;
    Vector2 textureCoord;
    Vector3 normal;

    MeshVertex();
    MeshVertex(const MeshVertex& vertex);
    MeshVertex(const Vector3& pos, const Vector2& tex,
               const Math::Vector3& norm);
};

struct MeshTriangle {
    UINT vertex0;
    UINT vertex1;
    UINT vertex2;

    MeshTriangle(UINT v0, UINT v1, UINT v2);
};

class MeshBuilder {
  private:
    // Device interface for creating GPU resources
    // Set by ResourceManager
    friend class ResourceManager;
    static ID3D11Device* device;

    std::vector<MeshVertex> vertex_buffer;
    std::vector<MeshTriangle> index_buffer;

  public:
    MeshBuilder();
    ~MeshBuilder();

    // Generates the Mesh for use in the rendering pipeline
    Mesh* generate();

    // Add a vertex to the builder, and returns the index corresponding to
    // this vertex.
    UINT addVertex(const Vector3& pos, const Vector2& tex, const Vector3& norm);

    // Add a triangle to the builder, based on indices.
    void addTriangle(UINT v1, UINT v2, UINT v3);

    // Discard the current normals for the mesh and regenerate them
    void regenerateNormals();

    // Resets the builder, so it can be used to generate another mesh
    void reset();

  private: 
    ID3D11Buffer* createVertexStream(void (*data_parser)(const MeshVertex&, uint8_t *output), UINT element_size);

    static void ExtractVertexPosition(const MeshVertex& vertex, uint8_t*output);
    static void ExtractVertexTexture(const MeshVertex& vertex, uint8_t* output);
    static void ExtractVertexNormal(const MeshVertex& vertex, uint8_t* output);
};

} // namespace Graphics
} // namespace Engine