#pragma once

#include <memory>

#include "rendering/core/Mesh.h"
#include "rendering/resources/ResourceManager.h"

namespace Engine {
namespace Graphics {
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
    create(SDFGeneratorDelegate* sdfGenerator,
           ResourceManager* resourceManager);
    ~TerrainOctree();

    void resetOctree(unsigned int _maxDepth, float _voxelSize);

    void update(const Vector3& pointOfFocus);
    void pullTerrainMeshes(std::vector<Mesh*>& meshes);

  private:
    TerrainOctree();
    std::unique_ptr<TerrainOctreeImpl> mImpl;
};

} // namespace Graphics
} // namespace Engine