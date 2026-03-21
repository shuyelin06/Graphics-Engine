#pragma once

#include <stdint.h>

#include "../core/DMCore.h"

#include "../Object.h"

#include "TerrainConfig.h"

namespace Engine {
namespace Datamodel {
// Terrain Class:
// Datamodel representation of the terrain in a scene.
class Terrain : public Object {
  private:
    DMTrackedProperty<uint32_t> seed;

  public:
    Terrain();
    ~Terrain();

    void propertyDisplay() override;
};

} // namespace Datamodel
} // namespace Engine
