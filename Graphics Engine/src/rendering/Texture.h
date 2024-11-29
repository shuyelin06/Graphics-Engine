#pragma once

#include "Direct3D11.h"

namespace Engine {
namespace Graphics {
// Texture Class:
// Textures store collections of data, which can be
// passed into and read from / written to by shaders.
// This class provides a unified interface for working
// with textures and passing them into the pipeline.
class Texture {
  private:
    ID3D11Texture2D* texture;

  public:
    Texture(ID3D11Texture2D* _texture);
    ~Texture();
};
} // namespace Graphics
} // namespace Engine