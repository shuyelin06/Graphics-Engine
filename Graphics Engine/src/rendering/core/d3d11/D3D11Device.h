#pragma once

#include "../Device.h"

#include "rendering/Direct3D11.h"

#include "D3D11PassTracker.h"
#include "D3D11ShaderCompiler.h"

namespace Engine
{
namespace Graphics
{
class Direct3D11Device : public Device
{
  private:
    ID3D11Device* device;

    std::unique_ptr<D3D11ShaderCompiler> shaders;

  public:
    Direct3D11Device(ID3D11Device* device);
    ~Direct3D11Device();

    D3D11ShaderCompiler* getShaders() { return shaders.get(); }

    ID3D11Device* getDevice() override;

    void reloadShaders() override;

    std::shared_ptr<Buffer> createBuffer(const char* debugName,
                                         BufferType type,
                                         size_t byteSize,
                                         const void* initData,
                                         bool dynamic) override;

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

    std::unique_ptr<D3D11PassTracker> queries;

    // Shaders
    D3D11ShaderCompiler* shaders = nullptr;
    D3D11VertexShader* vsActive = nullptr;
    D3D11PixelShader* psActive = nullptr;

    // Screen Target
    IDXGISwapChain* swapchain;
    ID3D11RenderTargetView* screenTarget;
    D3D11_VIEWPORT screenViewport;

    ID3D11Buffer* vertexCB[kVertexConstantBufferMax] = {nullptr};
    ID3D11Buffer* pixelCB[kVertexConstantBufferMax] = {nullptr};

    ID3D11Buffer* vb_buffers[BINDABLE_STREAM_COUNT] = {nullptr};
    unsigned int vb_strides[BINDABLE_STREAM_COUNT] = {0};
    unsigned int vb_offsets[BINDABLE_STREAM_COUNT] = {0};

    ID3D11DepthStencilState* depth_states[(int)DepthSettings::DepthFlagCount];
    ID3D11BlendState* blend_states[(int)BlendSettings::BlendFlagCount];
    ID3D11SamplerState* samplers[(int)SamplerSettings::SamplerCount];

  public:
    Direct3D11DeviceContext(ID3D11DeviceContext* context,
                            ID3D11Device* device,
                            IDXGISwapChain* swapchain,
                            ID3D11RenderTargetView* target,
                            D3D11_VIEWPORT viewport,
                            D3D11ShaderCompiler* shaders);
    ~Direct3D11DeviceContext();

    // Temporary and should be removed
    ID3D11DeviceContext* getContext() override;

    // Resources
    void updateBuffer(const std::shared_ptr<Buffer>& buffer,
                      const void* src,
                      size_t bytes) override;
    void updateTexture(const std::shared_ptr<Texture>& texture,
                       uint8_t slice,
                       const void* src,
                       size_t bytes) override;
    void generateMips(const std::shared_ptr<Texture>& texture) override;

    void clearRenderTarget(const std::shared_ptr<Texture>& texture,
                           const float rgba[4]) override;
    void clearDepthStencil(const std::shared_ptr<Texture>& texture) override;

    // Rendering
    void beginFrame(uint64_t frame) override;
    void endFrame() override;
    void beginPass(const char* passName) override;
    void endPass() override;
    const PassStats& getPassStats() override;

    void bindRenderTarget(const std::shared_ptr<Texture>& target,
                          const std::shared_ptr<Texture>& depth,
                          DepthSettings depthFlags,
                          BlendSettings blendFlags) override;
    void bindShaderProgram(const char* vs, const char* ps) override;

    // Vertex Shader Options:
    void loadVertexCB(uint8_t slot, const void* data, size_t bytes) override;
    void bindVertexTexture(uint8_t slot,
                           const std::shared_ptr<Texture>& texture,
                           SamplerSettings sampler) override;

    // Pixel Shader Options:
    void loadPixelCB(uint8_t slot, const void* data, size_t bytes) override;
    void bindPixelTexture(uint8_t slot,
                          const std::shared_ptr<Texture>& texture,
                          SamplerSettings sampler) override;

    void draw(const Geometry* geometry,
              uint32_t instanceCount,
              VertexTopology toplogy) override;

    void present() override;

  private:
    void initializeDepthStates();
    void initializeBlendStates();
    void initializeSamplers();

    void loadConstantBuffer(ID3D11Buffer** bufferPtr,
                            const void* data,
                            size_t bytes);
};

} // namespace Graphics
} // namespace Engine