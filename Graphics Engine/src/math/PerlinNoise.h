#pragma once

namespace Engine {
namespace Math {

// PerlinNoise Class:
// Contains methods for generating Perlin Noise.
// Adapted from https://adrianb.io/2014/08/09/perlinnoise.html
class PerlinNoise {
  public:
    static float octaveNoise2D(float x, float y, int octaves, float persistance);
    static float noise2D(float x, float y);
};

} // namespace Math
} // namespace Engine