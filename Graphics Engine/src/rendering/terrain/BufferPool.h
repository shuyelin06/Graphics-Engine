#pragma once

#include "../Direct3D11.h"
#include "../resources/AssetBuilder.h"
#include "../core/Asset.h"

namespace Engine {
namespace Graphics {
// BufferPool Class
// Manages mesh data for a collection of chunks at once, to reduce the number of
// draw calls needed for the terrain.

// BufferAllocation Struct:
// Stores the vertex/index buffer bytes that a terrain chunk
// is allocated in.
struct BufferAllocation {
    uint32_t vertex_start;
    uint32_t vertex_offset;

    uint32_t index_start;
    uint32_t index_offset;

    bool valid;
};

class BufferPool {
  private:
    // CPU-Side Data
    uint32_t v_size;       // Total # Vertices the Pool Supports
    uint32_t vertex_count; // Current # Vertices in the Pool
    Vector3* positions;    // Position Data
    Vector3* normals;      // Normal Data

    uint32_t i_size;         // Total # Triangles the Pool Supports
    uint32_t index_count;    // Current # Triangles
    MeshTriangle* triangles; // Triangle Data

    std::vector<BufferAllocation*> allocations;

    // GPU-Side Data
    ID3D11Buffer* b_position;
    ID3D11Buffer* b_normals;
    ID3D11Buffer* b_index;

  public:
    BufferPool(ID3D11Device* device, uint32_t vbuffer_size,
               uint32_t ibuffer_size);
    ~BufferPool();

    // Allocate / deallocate mesh data on the buffers. Set
    // BufferAllocation.valid to false to deallocate a mesh in the buffer.
    BufferAllocation* allocate(const std::vector<MeshVertex>& vertices,
                               const std::vector<MeshTriangle>& indices);

    // Call before an upload to the GPU.
    // Compacts the buffer data, removing invalid allocations.
    // This will resolve fragmentation.
    void cleanAndCompact();

    // Upload data to the GPU
    void updateGPUResources(ID3D11DeviceContext* context);

    // Retrieve GPU render information
    uint32_t getNumTriangles() const;

    ID3D11Buffer* getPositionBuffer() const;
    ID3D11Buffer* getNormalBuffer() const;
    ID3D11Buffer* getIndexBuffer() const;
};

}
} // namespace Engine