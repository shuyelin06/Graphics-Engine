#pragma once

#include <memory>

#include "rendering/core/Mesh.h"
#include "rendering/resources/ResourceManager.h"

namespace Engine {
namespace Graphics {
class RenderManager;

// TerrainOctree
// Implementation of an octree, which divides 3D space into recursively
// subdivided cubes.
class TerrainOctreeImpl;

class TerrainOctree {
  public:
    class SDFGeneratorDelegate {
      public:
        virtual float sampleSDF(float x, float y, float z) const = 0;
    };

    static std::unique_ptr<TerrainOctree>
    create(SDFGeneratorDelegate* sdfGenerator, ResourceManager* resourceManager,
           RenderManager* renderManager);
    ~TerrainOctree();

    void resetOctree(unsigned int _maxDepth, float _voxelSize);

    void updateMeshLODs(const Vector3& pointOfFocus);
    void serveBuildRequests();
    void updateDrawBlocks();

  private:
    TerrainOctree();
    std::unique_ptr<TerrainOctreeImpl> mImpl;
};

} // namespace Graphics
} // namespace Engine