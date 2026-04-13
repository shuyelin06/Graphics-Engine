#include "PSTerrain.h"

namespace Engine {
namespace Graphics {
PSTerrain::PSTerrain() {}

void PSTerrain::bind(Pipeline* pipeline) {
    pipeline->bindPixelShader("Terrain");
}

} // namespace Graphics
} // namespace Engine