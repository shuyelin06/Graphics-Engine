#include "RenderRequest.h"

namespace Engine {
namespace Graphics {
AssetRenderRequest::AssetRenderRequest(AssetSlot _slot,
                                       const Matrix4& _mLocalToWorld) {
    slot = _slot;
    mLocalToWorld = _mLocalToWorld;
}

TerrainData::TerrainData() = default;
TerrainData::TerrainData(
    float (*_data)[TERRAIN_CHUNK_X_SAMPLES][TERRAIN_CHUNK_Z_SAMPLES]
                  [TERRAIN_CHUNK_Y_SAMPLES]) {
    data = _data;
}
float TerrainData::sample(int x, int y, int z) { return (*data)[x][z][y]; }

TerrainRenderRequest::TerrainRenderRequest(int _x, int _z, TerrainData _data) {
    x_offset = _x;
    z_offset = _z;
    data = _data;
}

} // namespace Graphics
} // namespace Engine
