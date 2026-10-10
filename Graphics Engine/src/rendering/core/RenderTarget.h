#pragma once

#include "Texture.h"

namespace Engine
{
namespace Graphics
{
// Lightweight wrapper for a texture that can be used to bind it
// as a render target (or depth stencil).
// Create on demand with the device
enum class RenderTargetType
{
    Color = 0,
    DepthStencil = 1
};

class RenderTarget
{
  private:
    RenderTargetType type;

    uint32_t width;
    uint32_t height;

  public:
    RenderTarget(RenderTargetType type, uint32_t width, uint32_t height)
        : type(type)
        , width(width)
        , height(height)
    {
    }
    virtual ~RenderTarget() {}

    RenderTargetType getRenderTargetType() const { return type; }
    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
};

} // namespace Graphics
} // namespace Engine