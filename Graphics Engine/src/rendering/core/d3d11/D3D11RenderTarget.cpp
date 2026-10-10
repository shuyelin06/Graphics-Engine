#include "D3D11RenderTarget.h"

#include <assert.h>

namespace Engine
{
namespace Graphics
{
D3D11RenderTarget::D3D11RenderTarget(ID3D11Device* device,
                                     const D3D11Texture* texture,
                                     RenderTargetType type)
    : RenderTarget(type, texture->width, texture->height)
{
    if (type == RenderTargetType::DepthStencil)
    {
        assert(texture->mips == 1);
        D3D11_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
        dsDesc.Format =
            TextureLayoutToDXGI(texture->layout, TextureUsage::DepthStencil);
        if (texture->slices > 1)
        {
            dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsDesc.Texture2DArray.MipSlice = 0;
            dsDesc.Texture2DArray.FirstArraySlice = 0;
            dsDesc.Texture2DArray.ArraySize = texture->slices;
        }
        else
        {
            dsDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            dsDesc.Texture2D.MipSlice = 0;
        }
        HRESULT result = device->CreateDepthStencilView(texture->texture,
                                                        &dsDesc, &depthView);
        assert(SUCCEEDED(result));
    }
    else if (type == RenderTargetType::Color)
    {
        D3D11_RENDER_TARGET_VIEW_DESC targetDesc = {};
        targetDesc.Format =
            TextureLayoutToDXGI(texture->layout, TextureUsage::RenderTarget);
        if (texture->slices > 1)
        {
            targetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            targetDesc.Texture2DArray.MipSlice = 0;
            targetDesc.Texture2DArray.FirstArraySlice = 0;
            targetDesc.Texture2DArray.ArraySize = texture->slices;
        }
        else
        {
            targetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            targetDesc.Texture2D.MipSlice = 0;
        }
        HRESULT result = device->CreateRenderTargetView(
            texture->texture, &targetDesc, &targetView);
        assert(SUCCEEDED(result));
    }
    else
        assert(false);
}

D3D11RenderTarget::D3D11RenderTarget(ID3D11RenderTargetView* targetView,
                                     uint32_t width,
                                     uint32_t height)
    : targetView(targetView)
    , RenderTarget(RenderTargetType::Color, width, height)
{
}

D3D11RenderTarget ::~D3D11RenderTarget()
{
    if (depthView)
        depthView->Release();

    if (targetView)
        targetView->Release();
}

} // namespace Graphics
} // namespace Engine