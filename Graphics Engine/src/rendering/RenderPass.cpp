#include "RenderPass.h"

#include "datamodel/terrain/TerrainConfig.h"

namespace Engine {
namespace Graphics {
RenderPassTerrain::RenderPassTerrain(ID3D11Device* device) {
    sb_chunks.initialize(device, TERRAIN_CHUNK_COUNT * TERRAIN_CHUNK_COUNT *
                                     TERRAIN_CHUNK_COUNT);
    sb_indices.initialize(device, 200000 * 3);
    sb_positions.initialize(device, 300000);
    sb_normals.initialize(device, 300000);

    num_active_chunks = 0;
    max_chunk_triangles = 0;
}

} // namespace Graphics
} // namespace Engine