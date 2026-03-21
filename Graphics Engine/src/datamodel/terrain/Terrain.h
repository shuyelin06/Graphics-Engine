#pragma once

#include <climits>
#include <memory>
#include <queue>
#include <vector>

#include "../Bindable.h"
#include "../Object.h"

#include "TerrainConfig.h"
#include "datamodel/bvh/BVH.h"
#include "datamodel/bvh/TLAS.h"

#include "math/Triangle.h"

#include "math/Vector2.h"
#include "math/Vector3.h"

#include "math/PerlinNoise.h"

namespace Engine {
using namespace Math;

namespace Datamodel {
typedef unsigned int UINT;
class TerrainGenerator;

// Terrain Class:
// Datamodel representation of the terrain in a scene.
class Terrain : public Object {
  private:
    std::unique_ptr<TerrainGenerator> generator;

  public:
    Terrain();
    ~Terrain();

    void propertyDisplay() override;

    // --- Accessors ---
    TerrainGenerator& getGenerator() const;

    void seed(unsigned int seed);
};

} // namespace Datamodel
} // namespace Engine
