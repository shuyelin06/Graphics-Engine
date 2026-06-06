#pragma once

#include "TerrainOctree.h"

namespace Engine {
using namespace Math;
namespace Graphics {
class TerrainSDFImpl;
class TerrainSDF : public TerrainOctree::SDFGeneratorDelegate {
  public:
    static std::unique_ptr<TerrainSDF> create();
    ~TerrainSDF();

    void seedGenerator(unsigned int new_seed);

    // TerrainOctree::SDFGeneratorDelegate Implementation
    float sampleSDF(float x, float y, float z) const override;

  private:
    std::unique_ptr<TerrainSDFImpl> mImpl;
    TerrainSDF();
};

} // namespace Graphics
} // namespace Engine