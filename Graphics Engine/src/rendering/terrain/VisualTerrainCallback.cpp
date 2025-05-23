#include "VisualTerrainCallback.h"

#include "datamodel/terrain/Terrain.h"

// Hash Function for Vector3
template <> struct std::hash<Engine::Math::Vector3> {
    std::size_t operator()(const Engine::Math::Vector3& k) const {
        // https://stackoverflow.com/questions/5928725/hashing-2d-3d-and-nd-vectors
        uint32_t hash = std::_Bit_cast<uint32_t, float>(k.x) * 73856093 ^
                        std::_Bit_cast<uint32_t, float>(k.y) * 19349663 ^
                        std::_Bit_cast<uint32_t, float>(k.z) * 83492791;

        return hash % SIZE_MAX;
    }
};

namespace Engine {
using namespace Datamodel;

namespace Graphics {
// --- VisualTerrainCallback ---
VisualTerrainCallback::VisualTerrainCallback() = default;

void VisualTerrainCallback::initialize(ID3D11Device* _device) {
    device = _device;
}

// LoadMesh:
// Loads the MeshBuilder's data into a buffer pool.
BufferAllocation* VisualTerrainCallback::loadMesh(BufferPool& pool) {
    std::unique_lock<std::mutex> lock(mutex);
    dirty = false;
    if (indices.size() > 0)
        return pool.allocate(vertices, indices);
    else
        return nullptr;
}

// ExtractMesh:
// Pulls the mesh from the callback function, and sets its reference to nullptr.
Mesh* VisualTerrainCallback::extractMesh() {
    std::unique_lock<std::mutex> lock(mutex);
    Mesh* mesh = output_mesh;
    output_mesh = nullptr;

    dirty = false;

    return mesh;
}

// IsDirty:
// Returns if the callback has been updated or not.
bool VisualTerrainCallback::isDirty() {
    std::unique_lock<std::mutex> lock(mutex);
    return dirty;
}

// ReloadTerrainData:
// Callback function. When a terrain chunk is reloaded, reloads the chunk's mesh
// and stores it in output_mesh for the visual terrain interface to use.
void VisualTerrainCallback::reloadTerrainData(const TerrainChunk* chunk_data) {
    builder.reset();

    // Hashes vertices to their index in the mesh builder, so that we can reuse
    // vertices to get smooth normals.
    std::unordered_map<Vector3, UINT> vertex_map;
    vertex_map.clear();

    // Iterate through my chunk's triangles, and add them to the builder.
    UINT i0, i1, i2;
    for (const Triangle& triangle : chunk_data->triangles) {
        const Vector3 v0 = triangle.vertex(0);
        if (vertex_map.contains(v0))
            i0 = vertex_map[v0];
        else {
            i0 = builder.addVertex(v0);
            vertex_map[v0] = i0;
        }

        const Vector3 v1 = triangle.vertex(1);
        if (vertex_map.contains(v1))
            i1 = vertex_map[v1];
        else {
            i1 = builder.addVertex(v1);
            vertex_map[v1] = i1;
        }

        const Vector3 v2 = triangle.vertex(2);
        if (vertex_map.contains(v2))
            i2 = vertex_map[v2];
        else {
            i2 = builder.addVertex(v2);
            vertex_map[v2] = i2;
        }

        builder.addTriangle(i0, i1, i2);
    }

    // Add my chunk's border triangles, and add them to the builder.
    // These triangles are present only to make the normals consistent along the
    // borders.
    for (const Triangle& triangle : chunk_data->border_triangles) {
        const Vector3 v0 = triangle.vertex(0);
        if (vertex_map.contains(v0))
            i0 = vertex_map[v0];
        else {
            i0 = builder.addVertex(v0);
            vertex_map[v0] = i0;
        }

        const Vector3 v1 = triangle.vertex(1);
        if (vertex_map.contains(v1))
            i1 = vertex_map[v1];
        else {
            i1 = builder.addVertex(v1);
            vertex_map[v1] = i1;
        }

        const Vector3 v2 = triangle.vertex(2);
        if (vertex_map.contains(v2))
            i2 = vertex_map[v2];
        else {
            i2 = builder.addVertex(v2);
            vertex_map[v2] = i2;
        }

        builder.addTriangle(i0, i1, i2);
    }

    // Generate our normals, and remove the border triangles so that we don't
    // have z-fighting
    builder.regenerateNormals();
    builder.popTriangles(chunk_data->border_triangles.size());

    // Create our mesh.
    // If there is already a mesh, destroy it and replace it.
    {
        std::unique_lock<std::mutex> lock(mutex);

        dirty = true;

        vertices.assign(builder.getVertices().begin(),
                        builder.getVertices().end());
        indices.assign(builder.getIndices().begin(),
                       builder.getIndices().end());

        if (output_mesh != nullptr)
            delete output_mesh;
        output_mesh = builder.generateMesh(device);
    }
}

} // namespace Graphics
} // namespace Engine