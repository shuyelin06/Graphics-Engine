#pragma once

#include "TextureBuilder.h"

namespace Engine {
namespace Graphics {
// BumpMapBuilder Class:
// An extended texture builder class, that supports bump map creation.
// After loading a height map into the builder, call generate() to generate a
// texture that has normals encoded into its RGB channels.
class BumpMapBuilder : public TextureBuilder {
  private:
    using TextureBuilder::reset;

  protected:
    std::vector<float> heightmap;

  public:
    BumpMapBuilder(unsigned int width, unsigned int height);
    ~BumpMapBuilder();

    // Generates the bumpmap texture.
    Texture* generate(ID3D11Device* device);

    // Encode a height value in the heightmap
    void setHeight(int x, int y, float height);

  private:
    int index(int x, int y);
};

} // namespace Graphics
} // namespace Engine