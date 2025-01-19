#pragma once

#include <string>
#include <unordered_map>

#include "Direct3D11.h"

#include "rendering/core/TextureBuilder.h"
#include "rendering/core/Texture.h"
#include "utility/IndexMap.h"

namespace Engine {
using namespace Utility;

namespace Graphics {
// TextureManager Class:
// Manager for handling textures. It supports creation of textures, loading of
// textures, and will track them to ensure all textures are properly freed.
class TextureManager {
  private:
    ID3D11Device* device;

    std::unordered_map<std::string, Texture*> textures;

  public:
    TextureManager(ID3D11Device* device);

    // Texture getters
    Texture* getTexture(const std::string& name);

    // Built-in texture generation methods
    Texture* createDepthTexture(const std::string&, UINT width, UINT height);
    Texture* createShadowTexture(const std::string& name, UINT width, UINT height);

private:
    bool registerTexture(const std::string& name, Texture* texture);
};

} // namespace Graphics
} // namespace Engine