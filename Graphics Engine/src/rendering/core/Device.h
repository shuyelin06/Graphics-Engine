#pragma once

#include "Buffer.h"
#include "Geometry.h"
#include "Texture.h"

#include <memory>

#include "RenderSettings.h"

#include <Windows.h>
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;

namespace Engine
{
namespace Graphics
{
class Device;
class DeviceContext;

// VertexTopology Enum:
// Specifies how the vertices are arranged.
enum class VertexTopology : uint8_t
{
    TriangleList = 0,
    LineList = 1
};

enum class DepthSettings : uint8_t
{
    // Prevents the depth stencil from being bound
    Depth_Disabled = 0,
    // Enables the depth stencil and z-testing, but does not update the depth
    // value. The depth stencil can be read from in the shader while this flag
    // is set.
    Depth_TestNoWrite = 1,
    // Enables the depth stencil and z-testing, and updates the depth value
    // as well. The depth stencil can not be read from in the shader while set.
    Depth_TestAndWrite = 2,
    DepthFlagCount
};

enum class BlendSettings : uint8_t
{
    // Blending is done only off of the source alpha. For example, if srcA =
    // 0.7,
    // 70% of the color will be from the shader, and 30% from the render target
    SrcAlphaOnly = 0,
    // Blending is done off the source and destination alpha. For example, if
    // srcA = 0.3,
    // destA = 0.7, 30% of the color will be from the shader, and 70% from the
    // render target
    Blend_UseSrcAndDest = 1,
    BlendFlagCount,
    // ...
    Blend_Default = SrcAlphaOnly
};

// SamplerSlot Enum:
// Different samplers the pipeline supports.
enum class SamplerSettings : uint8_t
{
    Point = 0,
    Shadow = 1,
    Linear = 2,
    // Anisotrophic = 3,
    // Note: Additional samplers can be added here
    SamplerCount
};

void InitializeGraphicsAPI(HWND window,
                           std::unique_ptr<Device>& outDevice,
                           std::unique_ptr<DeviceContext>& outContext);

struct PassStats
{
    struct PassInfo
    {
        std::string_view name;
        float frameTime;
    };

    std::vector<PassInfo> stats;
    float totalFrameTime = 0.f;
    uint64_t frame = 0;
};

class DeviceContext
{
  public:
    DeviceContext() {};
    virtual ~DeviceContext() {};

    // Temporary and should be removed
    virtual ID3D11DeviceContext* getContext() = 0;

    // Resources
    virtual void updateBuffer(const std::shared_ptr<Buffer>& buffer,
                              const void* src,
                              size_t bytes) = 0;
    virtual void updateTexture(const std::shared_ptr<Texture>& texture,
                               uint8_t slice,
                               const void* src,
                               size_t bytes) = 0;
    virtual void generateMips(const std::shared_ptr<Texture>& texture) = 0;

    virtual void clearRenderTarget(const std::shared_ptr<Texture>& texture,
                                   const float rgba[4]) = 0;
    virtual void clearDepthStencil(const std::shared_ptr<Texture>& texture) = 0;

    // Rendering:
    virtual void beginFrame(uint64_t frame) = 0;
    virtual void endFrame() = 0;

    virtual void beginPass(const char* passName) = 0;
    virtual void endPass() = 0;

    virtual const PassStats& getPassStats() = 0;

    // Set target == null to use the screen space render target
    virtual void bindRenderTarget(const std::shared_ptr<Texture>& target,
                                  const std::shared_ptr<Texture>& depth,
                                  DepthSettings flags,
                                  BlendSettings blendFlags) = 0;
    virtual void bindShaderProgram(const char* vs, const char* ps) = 0;

    // Vertex Shader Options:
    virtual void loadVertexCB(uint8_t slot, const void* data, size_t bytes) = 0;
    virtual void bindVertexTexture(uint8_t slot,
                                   const std::shared_ptr<Texture>& texture,
                                   SamplerSettings sampler) = 0;

    // Pixel Shader Options:
    virtual void loadPixelCB(uint8_t slot, const void* data, size_t bytes) = 0;
    virtual void bindPixelTexture(uint8_t slot,
                                  const std::shared_ptr<Texture>& texture,
                                  SamplerSettings sampler) = 0;

    virtual void
    draw(const Geometry* geometry,
         uint32_t instanceCount,
         VertexTopology toplogy = VertexTopology::TriangleList) = 0;

    virtual void present() = 0;
};

class Device
{
  public:
    Device() {};
    virtual ~Device() {};

    // Temporary
    virtual ID3D11Device* getDevice() = 0;

    virtual void reloadShaders() = 0;

    virtual std::shared_ptr<Buffer> createBuffer(const char* debugName,
                                                 BufferType type,
                                                 size_t byteSize,
                                                 const void* initData,
                                                 bool dynamic) = 0;

    virtual std::shared_ptr<Texture>
    createTexture(const char* debugName,
                  TextureLayout layout,
                  TextureUsage usage,
                  uint32_t width,
                  uint32_t height,
                  uint16_t slices = 1,
                  uint8_t mips = 1,
                  bool dynamic = false,
                  const void* src = nullptr) = 0;
};

} // namespace Graphics
} // namespace Engine