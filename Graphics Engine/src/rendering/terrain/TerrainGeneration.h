#pragma once

#include <stdint.h>

#include "math/PerlinNoise.h"

#include "TerrainOctree.h"

namespace Engine {
using namespace Math;
namespace Graphics {
class TerrainGenerationImpl;
class TerrainGeneration : public TerrainOctree::SDFGeneratorDelegate {
  public:
    static std::unique_ptr<TerrainGeneration> create();
    ~TerrainGeneration();

    void seedGenerator(unsigned int new_seed);

    // TerrainOctree::SDFGeneratorDelegate Implementation
    float sampleSDF(float x, float y, float z) const override;

  private:
    std::unique_ptr<TerrainGenerationImpl> mImpl;
    TerrainGeneration();    
};

} // namespace Graphics
} // namespace Engine