#pragma once

#include <atomic>
#include <cstdint>

#include "core/Bitflags.h"

#include "rendering/Direct3D11.h"
#include "rendering/ImGui.h"

#include "math/Color.h"

typedef unsigned int UINT;

namespace Engine
{
using namespace Math;

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
};

inline static size_t TextureLayoutByteSize(TextureLayout layout)
{
    switch (layout)
    {
    case TextureLayout::R8G8B8A8_UNORM:
        [[fallthrough]];
    case TextureLayout::R32_FLOAT:
        return 4;

    default:
        return 0;
    }
}

// Texture Struct:
// Represents a texture that can be uploaded to the GPU.
struct Texture
{
    // GPU handle to the texture
    ID3D11Texture2D* texture = nullptr;

    // Debug Name
#if defined(IMGUI_ENABLED)
    std::string debugName{};
#endif

    // Texture descriptions
    unsigned int width, height; // Pixel width, height
    unsigned int mips = 1;      // # Mips
    TextureLayout layout;
    bool editable; // Can the texture be edited?

    // Different views for the texture. NULL if uninitialized.
    ID3D11ShaderResourceView* shader_view = nullptr;
    ID3D11DepthStencilView* depth_view = nullptr;
    ID3D11RenderTargetView* target_view = nullptr;

    std::atomic<bool> ready = false;

  public:
    Texture();
    Texture(ID3D11Texture2D* tex, UINT width, UINT height);
    Texture(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& desc);
    ~Texture();

#if defined(_DEBUG)
    void displayImGui() const;
    void displayImGui(float width) const;
#endif
};

} // namespace Graphics
} // namespace Engine