#pragma once

#include <memory>

#include "Texture.h"

// D3D11 Forward Declarations
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace Engine {
namespace Graphics {
class Device {
  private:
    ID3D11Device* device;

  public:
    Device(ID3D11Device*);
    ~Device();

    std::shared_ptr<Texture> createTexture(TextureLayout layout,
                                           unsigned int width,
                                           unsigned int height,
                                           unsigned int mips,
                                           bool dynamic = false);
};

class DeviceContext {
  private:
    ID3D11DeviceContext* context;

  public:
    DeviceContext(ID3D11DeviceContext*);
};

} // namespace Graphics
} // namespace Engine