#pragma once

#include "../RenderTarget.h"

#include "D3D11Texture.h"
#include "Direct3D11.h"

namespace Engine
{
namespace Graphics
{
class D3D11RenderTarget : public RenderTarget
{
  private:
    ID3D11DepthStencilView* depthView = nullptr;
    ID3D11RenderTargetView* targetView = nullptr;

  public:
    D3D11RenderTarget(ID3D11Device* device,
                      const D3D11Texture* texture,
                      RenderTargetType type,
                      uint8_t sliceIdx);
    D3D11RenderTarget(ID3D11RenderTargetView* targetView,
                      uint32_t width,
                      uint32_t height);
    ~D3D11RenderTarget();

    ID3D11DepthStencilView* getDepthView() { return depthView; }
    ID3D11RenderTargetView* getTargetView() { return targetView; }
};

} // namespace Graphics
} // namespace Engine