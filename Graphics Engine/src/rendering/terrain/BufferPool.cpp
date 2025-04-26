#include "BufferPool.h"

#include <assert.h>

#if defined(_DEBUG)
#include "../ImGui.h"
#endif

namespace Engine {
namespace Graphics {
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

#if defined(_DEBUG)
    ImGui::Text("Terrain Vertex Count: %u", vertex_count);
    ImGui::Text("Terrain Index Count: %u", index_count);
#endif
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

} // namespace Graphics
} // namespace Engine