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

    std::shared_ptr<Mesh> surface_mesh;
    int num_inner_tri;

    int num_waves;
    std::vector<WaveConfig> wave_config;

  public:
    WaterSurface();
    ~WaterSurface();

    void generateSurfaceMesh(std::shared_ptr<MeshBuilder> builder_ptr,
                             ID3D11DeviceContext* context, int width);
    void generateWaveConfig(int wave_count);

    const Mesh* getSurfaceMesh() const;
    int getNumInnerTriangles() const;
    int getNumWaves() const;
    const std::vector<WaveConfig>& getWaveConfig() const;

  private:
    void addQuad(MeshBuilder& builder, UINT a, UINT b, UINT c, UINT d);
};

} // namespace Graphics
} // namespace Engine