#pragma once

#include <atomic>
#include <cstdint>

#include "core/BitFlags.h"

#include "Resource.h"

namespace Engine
{
namespace Graphics
{
enum class TextureUsage : uint8_t
{
    ShaderResource = 1 << 0,
    DepthStencil = 1 << 1,
    RenderTarget = 1 << 2,
};

enum class TextureLayout : uint8_t
{
    R8G8B8A8_UNORM_SGRB = 0,
    R8G8B8A8_UNORM = 1,
    R32_FLOAT = 2, // 32-Bit Float from R Channel
    R24_UNORM_G8_UINT = 3,
    UNKNOWN
};

inline static size_t TextureLayoutByteSize(TextureLayout layout)
{
    switch (layout)
    {
    case TextureLayout::R8G8B8A8_UNORM_SGRB:
        [[fallthrough]];
    case TextureLayout::R8G8B8A8_UNORM:
        [[fallthrough]];
    case TextureLayout::R32_FLOAT:
        [[fallthrough]];
    case TextureLayout::R24_UNORM_G8_UINT:
        return 4;

    default:
        return 0;
    }
}

// Texture Struct:
// Represents a texture that can be uploaded to the GPU.
class Texture : public Resource
{
  public:
    std::atomic<bool> ready = false;

    Texture() {};
    virtual ~Texture() {};

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual uint16_t getArraySlices() const = 0;
    virtual uint8_t getMips() const = 0;
    virtual TextureLayout getTextureLayout() = 0;
};

} // namespace Graphics
} // namespace Engine