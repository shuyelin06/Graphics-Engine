#pragma once

#include "Texture.h"

#include <memory>

namespace Engine
{
namespace Graphics
{
class Device;
class DeviceContext;
struct DeviceImpl;
struct DeviceContextImpl;

void InitializeGraphicsAPI(HWND window,
                           std::unique_ptr<Device>& outDevice,
                           std::unique_ptr<DeviceContext>& outContext);

class DeviceContext
{
  private:
    friend void InitializeGraphicsAPI(HWND window,
                                      std::unique_ptr<Device>& device,
                                      std::unique_ptr<DeviceContext>& context);

    std::unique_ptr<DeviceContextImpl> mImpl;

  public:
    DeviceContext();
    ~DeviceContext();

    ID3D11DeviceContext* getContext();
    ID3D11RenderTargetView* getRenderTarget();

    void loadVertexCB(uint8_t slot, const void* data, size_t bytes);
    void loadPixelCB(uint8_t slot, const void* data, size_t bytes);

    void present();
};

class Device
{
  private:
    friend void InitializeGraphicsAPI(HWND window,
                                      std::unique_ptr<Device>& device,
                                      std::unique_ptr<DeviceContext>& context);

    std::unique_ptr<DeviceImpl> mImpl;

  public:
    Device();
    ~Device();

    // Temporary
    ID3D11Device* getDevice();

    std::shared_ptr<Texture> createTexture(TextureLayout layout,
                                           TextureUsage usage,
                                           unsigned int width,
                                           unsigned int height,
                                           unsigned int mips,
                                           const char* debugName,
                                           bool dynamic = false);
};

} // namespace Graphics
} // namespace Engine