#pragma once

#include "math/Vector3.h"

#include "../RenderManager.h"
#include "../StructuredBuffer.h"

namespace Engine {
using namespace Math;

namespace Graphics {
class TerrainMesh : public VertexTechnique {
  public:
    struct MeshDescription {
        unsigned int index_start;
        unsigned int index_count;

        unsigned int vertex_start;
        unsigned int vertex_count;
    };

  private:
    StructuredBuffer<MeshDescription> sb_descriptors;
    int num_active_chunks;
    int max_chunk_triangles;

    StructuredBuffer<unsigned int> sb_indices;
    StructuredBuffer<Vector3> sb_positions;
    StructuredBuffer<Vector3> sb_normals;

  public:
    TerrainMesh();

    void initialize(ID3D11Device* device, ID3D11DeviceContext* context);

    void uploadDescriptors(ID3D11DeviceContext* context,
                           const std::vector<MeshDescription>& descriptors);
    void uploadIndices(ID3D11DeviceContext* context, void* addr,
                       size_t numElements);
    void uploadPositions(ID3D11DeviceContext* context, void* addr,
                         size_t numElements);
    void uploadNormals(ID3D11DeviceContext* context, void* addr,
                       size_t numElements);

    // RenderMesh Implementation
    void bindAndDraw(Pipeline* pipeline, ID3D11DeviceContext* context,
                        PixelTechnique* pixelTechnique) override;
};

} // namespace Graphics
} // namespace Engine