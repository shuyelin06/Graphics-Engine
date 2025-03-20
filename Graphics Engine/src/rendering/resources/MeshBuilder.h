#pragma once

#include <vector>

#include "../core/Asset.h"
#include "math/Quaternion.h"

namespace Engine {
namespace Graphics {
// MeshBuilder Class:
// Enables creation of meshes. Meshes are represented by a vertex and index
// buffer. The vertex buffer stores all vertices in the mesh, and the index
// buffer references these vertices by index to create triangles for the mesh.
struct MeshVertex {
    Vector3 position;

    Vector2 tex;

    Vector3 normal;
    Color color;

    MeshVertex();
    MeshVertex(const Vector3& position, const Color& color);
    MeshVertex(const MeshVertex& vertex);
};

struct MeshTriangle {
    UINT vertex0;
    UINT vertex1;
    UINT vertex2;

    MeshTriangle(UINT v0, UINT v1, UINT v2);
};

class MeshBuilder {
  private:
    ID3D11Device* device;

    std::vector<MeshVertex> vertex_buffer;
    std::vector<MeshTriangle> index_buffer;

    Color active_color;

  public:
    MeshBuilder(ID3D11Device* device);
    ~MeshBuilder();

    // Generates the Mesh for use in the rendering pipeline
    Mesh* generate();

    // Set the active color
    void setColor(const Color& color);

    // Add vertices and triangles to the builder. If a vertex is added,
    // the builder returns the index corresponding to that vertex.
    UINT addVertex(const MeshVertex& vertex);
    UINT addVertex(const Vector3& pos);
    void addTriangle(UINT v1, UINT v2, UINT v3);
    
    UINT addVertices(const std::vector<MeshVertex>& vertices);
    void addTriangles(const std::vector<MeshTriangle>& indices, UINT start_index);

    // Return a MeshVertex from the builder (by index) for modification
    MeshVertex& getVertex(UINT index);

    // Add shapes to the builder. This makes it easy to compose objects using
    // the builder. Unit cube centered around the origin
    void addTriangle(const Vector3& a, const Vector3& b, const Vector3& c);
    void addCube(const Vector3& center, const Quaternion& rotation, float size);
    void addTube(const Vector3& start, const Vector3& end, float radius,
                 int num_vertices);

    // Discard the current normals for the mesh and regenerate them
    void regenerateNormals();

    // Resets the builder, so it can be used to generate another mesh
    void reset();

  private:
    ID3D11Buffer* createVertexStream(void (*data_parser)(const MeshVertex&,
                                                         uint8_t* output),
                                     UINT element_size);

    static void ExtractVertexPosition(const MeshVertex& vertex,
                                      uint8_t* output);
    static void ExtractVertexTexture(const MeshVertex& vertex, uint8_t* output);
    static void ExtractVertexNormal(const MeshVertex& vertex, uint8_t* output);
    static void ExtractVertexColor(const MeshVertex& vertex, uint8_t* output);
};

} // namespace Graphics
} // namespace Engine