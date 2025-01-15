#pragma once

#include "rendering/Direct3D11.h"

typedef unsigned int UINT;

namespace Engine {
namespace Graphics {

enum TextureFormat {

};

// Texture Struct:
// Represents a texture that can be uploaded to the GPU.
struct Texture {
    ID3D11Texture2D* texture;
    ID3D11ShaderResourceView* view;

    UINT width;
    UINT height;

#if defined(_DEBUG)
    void displayImGui();
#endif
};

} // namespace Graphics
} // namespace Engine