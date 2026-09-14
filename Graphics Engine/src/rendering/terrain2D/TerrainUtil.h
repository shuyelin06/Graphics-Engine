#pragma once

#include "rendering/resources/MeshBuilder.h"

namespace Engine
{
namespace Graphics
{
void generateTerrainPlaneMesh(MeshBuilder& builder,
                              int numSamples,
                              float skirtDepth);

}
} // namespace Engine