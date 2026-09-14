#pragma once

#include <memory>

#include "math/Vector3.h"

namespace Engine
{
using namespace Math;
namespace Graphics
{
class DeviceContext;
class VisualSystem;
class Terrain2DManagerImpl;
class Terrain2DManager
{
  public:
    static std::unique_ptr<Terrain2DManager> create(VisualSystem* visualSystem);
    ~Terrain2DManager();

    void updatePerform(const Vector3& cameraPosition, DeviceContext* context);

    void imGui();

  private:
    std::unique_ptr<Terrain2DManagerImpl> mImpl;
    Terrain2DManager();
};

} // namespace Graphics
} // namespace Engine