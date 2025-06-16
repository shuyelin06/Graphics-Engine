#pragma once

#include "../resources/AssetBuilder.h"

namespace Engine {
namespace Graphics {
// Wave Configuration:
// Stores the dimension, period, and amplitude for a wave.
// Can be directly passed into the shader
struct WaveConfig {
    Vector2 direction;
    float period;
    float amplitude;
};

// WaterSurface Class:
// Stores information regarding the terrain's water surface.
class WaterSurface {
  private:
    float size;

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

  private:
    // Determines how many subdivisions we need to make for the
    // water surface
    int levelOfDetail(float distance);
    void subdivideSurface(MeshBuilder& builder, UINT a, UINT b, UINT c, UINT d,
                          int depth);
};

} // namespace Graphics
} // namespace Engine