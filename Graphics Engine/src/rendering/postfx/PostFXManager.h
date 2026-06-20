#pragma once

#include <memory>

#include "rendering/VisualSystem.h"

namespace Engine {
namespace Graphics {
class PostFXManagerImpl;

// PostFXManager:
// Manages post processing effects for the engine
class PostFXManager {
  public:
    static std::unique_ptr<PostFXManager> create(VisualSystem* visualSystem);
    ~PostFXManager();

    void render();

  private:
    PostFXManager();
    std::unique_ptr<PostFXManagerImpl> mImpl;
};

} // namespace Graphics
} // namespace Engine