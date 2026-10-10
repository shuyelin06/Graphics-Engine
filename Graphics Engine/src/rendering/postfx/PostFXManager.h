#pragma once

#include <memory>

#include "math/Vector2.h"

#include "rendering/VisualSystem.h"

namespace Engine
{
namespace Graphics
{
class PostFXManagerImpl;

struct PostFXParams
{
    float resolutionX;
    float resolutionY;
    Vector2 padding;
};

// PostFXManager:
// Manages post processing effects for the engine
class PostFXManager
{
  public:
    static std::unique_ptr<PostFXManager> create(VisualSystem* visualSystem);
    ~PostFXManager();

    void render(DeviceContext* context);

    void drawPostFXQuad(DeviceContext* context);

    // Can be called by other systems
    void renderScreenspacePass(DeviceContext* context,
                               const PostFXParams& params,
                               const char* pixelShader);

  private:
    PostFXManager();
    std::unique_ptr<PostFXManagerImpl> mImpl;
};

} // namespace Graphics
} // namespace Engine