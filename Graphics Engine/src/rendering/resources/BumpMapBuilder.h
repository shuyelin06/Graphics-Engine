#pragma once

#include "TextureBuilder.h"

namespace Engine
{
namespace Graphics
{
// BumpMapBuilder Class:
// An extended texture builder class, that supports bump map creation.
// After loading a height map into the builder, call generate() to generate a
// texture that has normals encoded into its RGB channels.
class BumpMapBuilder : private TextureBuilder
{
  private:
    using TextureBuilder::reset;

  protected:
    std::vector<float> heightmap;

    unsigned int heightmap_width;
    unsigned int heightmap_height;

  public:
    BumpMapBuilder(unsigned int width, unsigned int height);
    ~BumpMapBuilder();

    // Encode a height value in the heightmap
    void setHeight(int x, int y, float height);
    // Create bump map with Perlin Noise
    void samplePerlinNoise(unsigned int seed, float freq, float amplitude);

  private:
    void computeNormals();
    int index(int x, int y);
};

} // namespace Graphics
} // namespace Engine