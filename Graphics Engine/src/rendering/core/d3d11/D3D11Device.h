#pragma once

#include "../Device.h"

#include "rendering/Direct3D11.h"

namespace Engine
{
namespace Graphics
{
class Direct3D11Device : public Device
{
  private:
    ID3D11Device* device;

  public:
    Direct3D11Device(ID3D11Device* device);
    ~Direct3D11Device();

    ID3D11Device* getDevice() override;

    std::shared_ptr<Buffer> createBuffer(const char* debugName,
                                         BufferType type,
                                         size_t byteSize,
                                         const void* initData) override;

    std::shared_ptr<Texture> createTexture(const char* debugName,
                                           TextureLayout layout,
                                           TextureUsage usage,
                                           uint32_t width,
                                           uint32_t height,
                                           uint16_t slices,
                                           uint8_t mips,
                                           bool dynamic,
                                           const void* src) override;
};

class Direct3D11DeviceContext : public DeviceContext
{
  private:
    ID3D11DeviceContext* context;
    ID3D11Device* device;

    // Screen Target
    IDXGISwapChain* swapchain;
    ID3D11RenderTargetView* screenTarget;
    D3D11_VIEWPORT screenViewport;

    ID3D11Buffer* vertexCB[kVertexConstantBufferMax] = {nullptr};
    ID3D11Buffer* pixelCB[kVertexConstantBufferMax] = {nullptr};

    ID3D11DepthStencilState*
        depth_states[(int)DepthStencilFlags::DepthFlagCount];
    ID3D11BlendState* blend_states[(int)BlendFlags::BlendFlagCount];
    ID3D11SamplerState* samplers[(int)SamplerType::SamplerCount];

  public:
    Direct3D11DeviceContext(ID3D11DeviceContext* context,
                            ID3D11Device* device,
                            IDXGISwapChain* swapchain,
                            ID3D11RenderTargetView* target,
                            D3D11_VIEWPORT viewport);
    ~Direct3D11DeviceContext();

    // Temporary and should be removed
    ID3D11DeviceContext* getContext() override;

    // Resources
    void updateTexture(const std::shared_ptr<Texture>& texture,
                       uint8_t slice,
                       const void* src,
                       size_t bytes) override;
    void generateMips(const std::shared_ptr<Texture>& texture) override;

    void clearRenderTarget(const std::shared_ptr<Texture>& texture,
                           const float rgba[4]) override;
    void clearDepthStencil(const std::shared_ptr<Texture>& texture) override;

    // Rendering
    void bindRenderTarget(const std::shared_ptr<Texture>& target,
                          const std::shared_ptr<Texture>& depth,
                          DepthStencilFlags depthFlags,
                          BlendFlags blendFlags) override;

    // Vertex Shader Options:
    void loadVertexCB(uint8_t slot, const void* data, size_t bytes) override;
    void bindVertexTexture(uint8_t slot,
                           const std::shared_ptr<Texture>& texture,
                           SamplerType sampler) override;

    // Pixel Shader Options:
    void loadPixelCB(uint8_t slot, const void* data, size_t bytes) override;
    void bindPixelTexture(uint8_t slot,
                          const std::shared_ptr<Texture>& texture,
                          SamplerType sampler) override;

    void present() override;

  private:
    void initializeDepthStates();
    void initializeBlendStates();
    void initializeSamplers();
};

} // namespace Graphics
} // namespace Engine