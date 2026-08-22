#pragma once

#include "../Texture.h"

#include "rendering/Direct3D11.h"

namespace Engine
{
namespace Graphics
{
class D3D11Texture : public Texture
{
  private:
    // Texture descriptions
    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t slices = 1;
    uint8_t mips = 1;
    TextureLayout layout = TextureLayout::UNKNOWN;
    bool dynamic = false;

    // GPU handle to the texture
    ID3D11Texture2D* texture = nullptr;
    // Different views for the texture. NULL if uninitialized.
    ID3D11ShaderResourceView* shader_view = nullptr;
    ID3D11DepthStencilView* depth_view = nullptr;
    ID3D11RenderTargetView* target_view = nullptr;

  public:
    D3D11Texture(ID3D11Device* device,
                 TextureLayout layout,
                 TextureUsage usage,
                 uint32_t width,
                 uint32_t height,
                 uint16_t slices,
                 uint8_t mips,
                 bool dynamic,
                 const void* src);
    ~D3D11Texture();

    uint32_t getWidth() const override { return width; }
    uint32_t getHeight() const override { return height; }
    uint16_t getArraySlices() const override { return slices; }
    uint8_t getMips() const override { return mips; }
    TextureLayout getTextureLayout() override { return layout; }

    ID3D11ShaderResourceView* getSRV() const { return shader_view; }
    ID3D11DepthStencilView* getDepthView() { return depth_view; }
    ID3D11RenderTargetView* getTargetView() { return target_view; }

    void update(ID3D11DeviceContext* context,
                uint8_t slice,
                const void* initData,
                size_t bytes);

#if defined(IMGUI_ENABLED)
    void doImgui() const override;
#endif
};

} // namespace Graphics
} // namespace Engine