#include "PSTerrain.h"

namespace Engine {
namespace Graphics {
PSTerrain::PSTerrain(LightManager* lightManager)
    : mLightManager(lightManager) {}

void PSTerrain::bind(Pipeline* pipeline, ID3D11DeviceContext* context) {
    // Pixel Constant Buffer 1: Light Data
    // Stores data that is needed for lighting / shadowing.
    {
        IConstantBuffer pCB1 = pipeline->loadPixelCB(CB1);
        mLightManager->bindLightData(pCB1);
    }
}

} // namespace Graphics
} // namespace Engine