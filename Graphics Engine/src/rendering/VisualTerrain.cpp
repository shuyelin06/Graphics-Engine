#include "VisualTerrain.h"

#include <assert.h>
#include <unordered_map>

#if defined(_DEBUG)
#include "ImGui.h"
#endif

// Hash Function for Vector3
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

// --- BufferPool Class ---
// Constructor:
// Creates the buffers that the pool manages.
BufferPool::BufferPool(ID3D11Device* device, uint32_t _vbuffer_size,
                       uint32_t _ibuffer_size) {
    D3D11_BUFFER_DESC buff_desc = {};

    // Create my vertex buffers
    v_size = _vbuffer_size;
    positions = new Vector3[v_size];
    normals = new Vector3[v_size];

    buff_desc.ByteWidth = v_size * sizeof(Vector3);
    buff_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    buff_desc.Usage = D3D11_USAGE_DYNAMIC;
    buff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&buff_desc, NULL, &b_position);

    buff_desc.ByteWidth = v_size * sizeof(Vector3);
    buff_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    buff_desc.Usage = D3D11_USAGE_DYNAMIC;
    buff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&buff_desc, NULL, &b_normals);

    // Create my index buffer
    i_size = _ibuffer_size;
    triangles = new MeshTriangle[i_size];

    buff_desc.ByteWidth = i_size * sizeof(MeshTriangle);
    buff_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    buff_desc.Usage = D3D11_USAGE_DYNAMIC;
    buff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&buff_desc, NULL, &b_index);

    vertex_count = index_count = 0;
}
BufferPool::~BufferPool() {
    delete[] positions;
    delete[] normals;
    delete[] triangles;

    if (b_position != nullptr)
        b_position->Release();
    if (b_normals != nullptr)
        b_normals->Release();
    if (b_index != nullptr)
        b_index->Release();
}

BufferAllocation*
BufferPool::allocate(const std::vector<MeshVertex>& vertices,
                     const std::vector<MeshTriangle>& indices) {
    // Create a new allocation immediately after our last allocation in the
    // buffer pool
    assert(index_count + indices.size() < i_size);
    assert(vertex_count + vertices.size() < v_size);

    BufferAllocation* new_alloc = new BufferAllocation();
    new_alloc->vertex_start = vertex_count;
    new_alloc->vertex_offset = vertices.size();
    new_alloc->index_start = index_count;
    new_alloc->index_offset = indices.size();
    new_alloc->valid = true;
    allocations.push_back(new_alloc);

    vertex_count += vertices.size();
    index_count += indices.size();

    // Write my data to our CPU-side memory
    int vindex = new_alloc->vertex_start;
    for (const MeshVertex& vertex : vertices) {
        positions[vindex] = vertex.position;
        normals[vindex] = vertex.normal;
        vindex++;
    }

    int iindex = new_alloc->index_start;
    for (const MeshTriangle& tri : indices) {
        const MeshTriangle transformed =
            MeshTriangle(tri.vertex0 + new_alloc->vertex_start,
                         tri.vertex1 + new_alloc->vertex_start,
                         tri.vertex2 + new_alloc->vertex_start);
        triangles[iindex] = transformed;
        iindex++;
    }

    // Return my allocation
    return new_alloc;
}

void BufferPool::cleanAndCompact() {
    // Iterate through our allocations, and erase any buffer allocations whose
    // valid flag is false
    std::vector<BufferAllocation*>::iterator iter = allocations.begin();

    // Stores the current end of the vertex vectors
    int v_end = 0;
    // Stores the current end of the index vector
    int i_end = 0;

    while (iter != allocations.end()) {
        BufferAllocation* alloc = (*iter);

        // If the allocation is invalid, then remove it from our vector
        // and do nothing else.
        if (!alloc->valid) {
            iter = allocations.erase(iter);
            delete alloc;
        }
        // If the allocation is valid, memcpy the allocation's vertices and
        // indices to the current end of the positions and normals list.
        else {
            const uint32_t new_vstart = v_end;
            const uint32_t new_istart = i_end;

            // Copy over our vertex data
            memcpy(positions + v_end, positions + alloc->vertex_start,
                   alloc->vertex_offset * sizeof(Vector3));
            memcpy(normals + v_end, normals + alloc->vertex_start,
                   alloc->vertex_offset * sizeof(Vector3));

            // Calculate the difference between the old vertex_start and the new
            // one. Then, shift the indices down, offsetting by this value (to
            // account for the shift in vertices). All indices in the index
            // buffer need to be offset by this value.
            const int difference = alloc->vertex_start - v_end;

            for (int i = 0; i < alloc->index_offset; i++) {
                const MeshTriangle& cur_tri = triangles[alloc->index_start + i];

                const MeshTriangle new_tri = MeshTriangle(
                    cur_tri.vertex0 - difference, cur_tri.vertex1 - difference,
                    cur_tri.vertex2 - difference);
                triangles[i_end] = new_tri;

                i_end++;
            }

            v_end += alloc->vertex_offset;

            // Update my vertex and index starts
            alloc->vertex_start = new_vstart;
            alloc->index_start = new_istart;

            iter++;
        }
    }

    vertex_count = v_end;
    index_count = i_end;

    ImGui::Text("Terrain Vertex Count: %u", vertex_count);
    ImGui::Text("Terrain Index Count: %u", index_count);
}

void BufferPool::updateGPUResources(ID3D11DeviceContext* context) {
    if (vertex_count > 0 && index_count > 0) {
        D3D11_MAPPED_SUBRESOURCE sr_position = {0};
        D3D11_MAPPED_SUBRESOURCE sr_normals = {0};
        D3D11_MAPPED_SUBRESOURCE sr_indices = {0};

        context->Map(b_position, 0, D3D11_MAP_WRITE_DISCARD, 0, &sr_position);
        memcpy(sr_position.pData, positions, vertex_count * sizeof(Vector3));
        context->Unmap(b_position, 0);

        context->Map(b_normals, 0, D3D11_MAP_WRITE_DISCARD, 0, &sr_normals);
        memcpy(sr_normals.pData, normals, vertex_count * sizeof(Vector3));
        context->Unmap(b_normals, 0);

        context->Map(b_index, 0, D3D11_MAP_WRITE_DISCARD, 0, &sr_indices);
        memcpy(sr_indices.pData, triangles, index_count * sizeof(MeshTriangle));
        context->Unmap(b_index, 0);
    }
}

uint32_t BufferPool::getNumTriangles() const { return index_count; }
ID3D11Buffer* BufferPool::getPositionBuffer() const { return b_position; }
ID3D11Buffer* BufferPool::getNormalBuffer() const { return b_normals; }
ID3D11Buffer* BufferPool::getIndexBuffer() const { return b_index; }

VisualTerrain::VisualTerrain(Terrain* _terrain, ID3D11Device* device)
    : terrain(_terrain), callbacks() {
    // Initialize our buffer pool. This will consolidate our mesh data
    // into a few vertex / index buffers, to dramatically reduce the number of
    // draw calls we make.
    // Empirical testing has shown that 300,000 vertices, 200,000 indices is
    // enough.
    output_mesh = new BufferPool(device, 300000, 200000);

    // Set all current chunk_meshes to null, and initialize my terrain
    // callbacks.
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                chunk_meshes[i][j][k] = nullptr;
                allocations[i][j][k] = nullptr;

                callbacks[i][j][k].initialize(device);
                terrain->registerTerrainCallback(i, j, k, &callbacks[i][j][k]);
            }
        }
    }
}
VisualTerrain::~VisualTerrain() {
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                if (chunk_meshes[i][j][k] != nullptr)
                    delete chunk_meshes[i][j][k];
            }
        }
    }
}

// GenerateTerrainMesh:
// Generates the mesh for the terrain
void VisualTerrain::pullTerrainMeshes(ID3D11DeviceContext* context) {
    // Iterate through my callbacks. If they have a mesh, overwrite what we
    // currently have.
    bool dirty = false;

    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                VisualTerrainCallback& callback = callbacks[i][j][k];

                if (callback.isDirty()) {
                    dirty = true;

                    if (chunk_meshes[i][j][k] != nullptr)
                        delete chunk_meshes[i][j][k];
                    chunk_meshes[i][j][k] = callback.extractMesh();

                    if (allocations[i][j][k] != nullptr) {
                        allocations[i][j][k]->valid = false;
                        allocations[i][j][k] = nullptr;
                    }
                    allocations[i][j][k] = callback.loadMesh(*output_mesh);
                }
            }
        }
    }

    // Now, iterate through all of my output meshes and add them to the output
    // vector.
    output_meshes.clear();
    for (int i = 0; i < TERRAIN_CHUNK_COUNT; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_COUNT; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_COUNT; k++) {
                if (chunk_meshes[i][j][k] != nullptr)
                    output_meshes.push_back(chunk_meshes[i][j][k]);
            }
        }
    }

    // CLEANING MAY CAUSE THE ALLOCATION POINTERS TO CAUSE A MEMORY
    // CORRUPTION...
    if (dirty) {
        output_mesh->cleanAndCompact();
        output_mesh->updateGPUResources(context);
    }
}

// GetTerrainMesh:
// Returns the terrain's mesh
const std::vector<Mesh*>& VisualTerrain::getTerrainMeshes() {
    return output_meshes;
}

BufferPool* VisualTerrain::getMesh() { return output_mesh; }

/*
static int generateTreeMeshHelper(MeshBuilder& builder,
                                  const std::vector<TreeStructure>& grammar,
                                  int index, const Vector3& position,
                                  const Vector2& rotation) {
    if (index >= grammar.size())
        return -1;

    const TreeStructure tree = grammar[index];

    switch (tree.token) {
    case TRUNK: {
        const float phi = rotation.u;
        const float theta = rotation.v;

        Vector3 direction = SphericalToEuler(1.0, theta, phi);
        const Quaternion rotation_offset =
            Quaternion::RotationAroundAxis(Vector3::PositiveX(), -PI / 2);
        direction = rotation_offset.rotationMatrix3() * direction;

        const Vector3 next_pos =
            position + direction * tree.trunk_data.trunk_length;

        builder.setColor(Color(150.f / 255.f, 75.f / 255.f, 0));
        builder.addTube(position, next_pos, tree.trunk_data.trunk_thickess, 5);
        return generateTreeMeshHelper(builder, grammar, index + 1, next_pos,
                                      rotation);
    } break;

    case BRANCH: {
        const Vector2 new_rotation =
            rotation + Vector2(tree.branch_data.branch_angle_phi,
                               tree.branch_data.branch_angle_theta);

        const int next_index = generateTreeMeshHelper(
            builder, grammar, index + 1, position, new_rotation);
        return generateTreeMeshHelper(builder, grammar, next_index, position,
                                      rotation);
    } break;

    case LEAF: {
        builder.setColor(Color::Green());

        const Vector3 random_axis =
            Vector3(1 + Random(0.f, 1.f), Random(0.f, 1.f), Random(0.f, 1.f))
                .unit();
        const float angle = Random(0, 2 * PI);

        builder.addCube(position,
                        Quaternion::RotationAroundAxis(random_axis, angle),
                        tree.leaf_data.leaf_density);
        return index + 1;
    } break;
    }

    return -1;
}

Mesh* VisualTerrain::generateTreeMesh(MeshBuilder& builder,
                                      const Vector2& location) {
    builder.reset();

    TreeGenerator gen = TreeGenerator();
    gen.generateTree();

    // Rotation stores (phi, theta), spherical angles. rho is assumed to be 1.
    const float x = location.u + terrain->getX();
    const float z = location.v + terrain->getZ();

    generateTreeMeshHelper(builder, gen.getTree(), 0,
                           Vector3(x, terrain->sampleTerrainHeight(x, z), z),
                           Vector2(0, 0));

    builder.regenerateNormals();

    return builder.generate();
}
*/

} // namespace Graphics
} // namespace Engine