#pragma once

#include <stdint.h>

#include "rendering/pipeline/PipelineManager.h"

namespace Engine {
namespace Graphics {
enum RenderPass {
    kShadows = 0,
    kOpaque = 1,
    kDebug = 2,
    _Count_,
};
struct RenderPassSet {
    uint32_t bitset;

    RenderPassSet() { bitset = 0; }
    RenderPassSet(uint32_t _bitset) { bitset = _bitset; }

    void addPass(RenderPass pass) { bitset |= (1 << pass); }
    void removePass(RenderPass pass) { bitset ^= (1 << pass); };
    bool hasPass(RenderPass pass) const { return bitset & (1 << pass); };
};
static_assert(RenderPass::_Count_ < 32);

enum class MeshType : uint8_t {
    kDefaultMesh = 0,
    kTerrain = 1,
};

class PixelTechnique {
  public:
    virtual void bind(Pipeline* pipeline) = 0;
};

class VertexTechnique {
  public:
    // Executes the draw call too?
    virtual void bindAndDraw(Pipeline* pipeline, RenderPass pass) = 0;
};

struct TerrainMeshMetadata {
    uint16_t maxChunkIndices;
    uint16_t numIndicesPerChunk;
    uint32_t padding;
};

struct DefaultMeshMetadata {
    uint16_t indexStart;
    uint16_t indexCount;
    uint16_t vertexStart;
    uint16_t padding;
};

// Stores all data needed for the pipeline to make a draw
// call.
// Ideally we want to move away from virtual calls if we can help it,
// but this will do for simplicity (for now).
// Data is stored in order of what is most --> least important
// when sorting draw calls. We do not want a separate sorting key as
// that would be inefficient.
using PixelTechniqueKey = uint16_t;
using VertexTechniqueKey = uint16_t;
struct DrawCall {
    uint32_t depth = 0xFF;

    PixelTechnique* pixelTechnique = nullptr;
    VertexTechnique* vertexTechnique = nullptr;

    union {
        TerrainMeshMetadata terrainData;
        DefaultMeshMetadata defaultData;
    } metadata;
    static_assert(sizeof(metadata) == 8);

    DrawCall() = default;
    DrawCall(VertexTechnique* _vertexTechnique,
             PixelTechnique* _pixelTechnique) {
        vertexTechnique = _vertexTechnique;
        pixelTechnique = _pixelTechnique;
    }
};

} // namespace Graphics
} // namespace Engine