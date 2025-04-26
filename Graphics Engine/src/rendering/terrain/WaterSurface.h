#pragma once

#include "../resources/AssetBuilder.h"

namespace Engine {
namespace Graphics {
// Wave Configuration:
// Stores the dimension, period, and amplitude for a wave.
// Can be directly passed into the shader
struct WaveConfig {
    int dimension;
    float period;
    float amplitude;
    float offset;
};

// WaterSurface Class:
// Stores information regarding the terrain's water surface.
class WaterSurface {
  private:
    Mesh* surface_mesh;

    int num_waves;
    std::vector<WaveConfig> wave_config;

  public:
    WaterSurface();
    ~WaterSurface();

    void generateSurfaceMesh(ID3D11Device* device, float size,
                             int num_subdivisions);
    void generateWaveConfig(int wave_count);

    const Mesh* getSurfaceMesh() const;
    int getNumWaves() const;
    const std::vector<WaveConfig>& getWaveConfig() const;
};

} // namespace Graphics
} // namespace Engine