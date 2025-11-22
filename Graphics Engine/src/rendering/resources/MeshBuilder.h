#pragma once

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../core/Mesh.h"
#include "math/Quaternion.h"

namespace Engine {
namespace Graphics {
// MeshBuilder Class:
// Enables creation of meshes. Meshes are represented by a vertex and index
// buffer. The vertex buffer stores all vertices in the mesh, and the index
// buffer references these vertices by index to create triangles for the mesh.
// A mesh vertex contains all properties a vertex can possibly have. It's about
// the same as the vertex streams defined in VertexStreamIDs.h
struct MeshVertex {
    Vector3 position;
    Vector2 tex;
    Vector3 normal;
    Color color;
    // No Debug_Line stream
    Vector4 joints;
    Vector4 weights;

    MeshVertex();
    MeshVertex(const Vector3& position, const Color& color);
    MeshVertex(const MeshVertex& vertex);

    void* GetAddressOf(VertexDataStream bindable_stream);
};

struct MeshTriangle {
    uint32_t vertex0;
    uint32_t vertex1;
    uint32_t vertex2;

    MeshTriangle();
    MeshTriangle(UINT v0, UINT v1, UINT v2);
};

class MeshBuilder {
  private:
    MeshPool* target_pool;

    uint16_t layout;

    std::vector<MeshVertex> vertex_buffer;
    std::vector<MeshTriangle> index_buffer;

  public:
    MeshBuilder(MeshPool* pool);
    MeshBuilder(const MeshBuilder& builder);
    ~MeshBuilder();

    // Generates the mesh for use in the rendering pipeline.
    // If no buffer_pool is provided, the builder will generate one for this
    // mesh.
    std::shared_ptr<Mesh> generateMesh(ID3D11DeviceContext* context,
                                       MeshPool* buffer_pool);

    // NEW-- DEPRECATE THE ABOVE
    std::shared_ptr<Mesh> generateMesh(ID3D11DeviceContext* context);

    const std::vector<MeshVertex>& getVertices() const;
    const std::vector<MeshTriangle>& getIndices() const;
    std::vector<MeshVertex>& getVertices();
    std::vector<MeshTriangle>& getIndices();

    bool isEmpty() const;

    // Add a stream to the builder's output
    void addLayout(VertexDataStream stream);

    // Add vertices and triangles to the builder. If a vertex is added,
    // the builder returns the index corresponding to that vertex.
    UINT addVertex(const MeshVertex& vertex);
    UINT addVertex(const Vector3& pos);
    void addTriangle(UINT v1, UINT v2, UINT v3);

    UINT addVertices(const std::vector<MeshVertex>& vertices);
    void addTriangles(const std::vector<MeshTriangle>& indices,
                      UINT start_index);
    void popTriangles(UINT num_triangles);

    // Return a MeshVertex from the builder (by index) for modification
    MeshVertex& getVertex(UINT index);

    Vector3& getPosition(UINT index);

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
    void uploadVertexData(ID3D11DeviceContext* context, ID3D11Buffer* buffer,
                          UINT buffer_size, void* (*addressor)(MeshVertex&),
                          UINT byte_size);
};

} // namespace Graphics
} // namespace Engine