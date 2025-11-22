#pragma once

#include "Mesh.h"
#include "Material.h"

namespace Engine
{
namespace Graphics
{
// Geometry:
// A mesh and a material. Geometry is the basic renderable type that can be submitted
// through the render pipeline.
// Various stages of the render pipeline will pull the data from this
struct Geometry
{
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
};

}
}