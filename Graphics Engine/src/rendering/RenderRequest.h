#pragma once

#include "AssetIDs.h"
#include "datamodel/TerrainConfig.h"
#include "math/Matrix4.h"

namespace Engine {
using namespace Math;
namespace Graphics {
// RenderRequest Class(es):
// Structures that represent render requests that are submitted
// to the visual system.
struct AssetRenderRequest {
    AssetSlot slot;
    Matrix4 mLocalToWorld;

    AssetRenderRequest(AssetSlot slot, const Matrix4& mLocalToWorld);
};

struct TerrainData {
    float (*data)[TERRAIN_CHUNK_X_SAMPLES][TERRAIN_CHUNK_Z_SAMPLES]
                 [TERRAIN_CHUNK_Y_SAMPLES];

    TerrainData();
    TerrainData(float (*data)[TERRAIN_CHUNK_X_SAMPLES][TERRAIN_CHUNK_Z_SAMPLES]
                             [TERRAIN_CHUNK_Y_SAMPLES]);

    float sample(int x, int y, int z);
};
struct TerrainRenderRequest {
    int x_offset;
    int z_offset;

    TerrainData data;

    TerrainRenderRequest(int x, int z, TerrainData data);
};

} // namespace Graphics
} // namespace Engine
