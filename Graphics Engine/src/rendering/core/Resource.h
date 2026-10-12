#pragma once

#include "rendering/ImGui.h"

namespace Engine
{
namespace Graphics
{
// Generic Resource Class. All GPU Resources will inherit this class
class Resource
{
  public:
    virtual ~Resource() {};
};

} // namespace Graphics
} // namespace Engine