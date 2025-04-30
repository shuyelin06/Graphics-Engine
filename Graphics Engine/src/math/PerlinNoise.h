#pragma once

namespace Engine {
namespace Math {

// PerlinNoise Class:
// Contains methods for generating Perlin Noise.
// Can be used to sample perlin noise. Uses a seed to randomly generate a
// permutation table, which will define the shape of the noise.
// Adapted from https://adrianb.io/2014/08/09/perlinnoise.html
class PerlinNoise {
  private:
    // The permutation table defines the "seed" for the PerlinNoise.
    unsigned char permutation_table[256];

  public:
    PerlinNoise(unsigned int seed);

    float noise2D(float x, float y) const;
    float octaveNoise2D(float x, float y, int octaves, float persistence) const;

    float noise3D(float x, float y, float z) const;

    const unsigned char* getPermutationTable();
  private:
    void seedGenerator(unsigned int seed);
    unsigned char indexTable(int index) const;
};

} // namespace Math
} // namespace Engine