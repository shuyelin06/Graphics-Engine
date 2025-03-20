#pragma once

#include <string>
#include <vector>

#include "../Direct3D11.h"
#include "Texture.h"
#include "VertexStreamIDs.h"

#include "math/AABB.h"
#include "math/Color.h"
#include "math/Vector2.h"
#include "math/Vector3.h"

namespace Engine {
using namespace Math;
namespace Graphics {
typedef unsigned int UINT;

// Struct Material:
// Specifies renderable properties for a mesh
struct Material {
    Color base_color;

    float diffuse_factor; // Roughness

  public:
    // Default material settings
    Material();
};

// Struct Mesh:
// Specifies a mesh, which is a collection of vertices that form triangles.
// Vertices are stored in separate vertex streams, so that they have an easier
// time being passed as input into shaders.
struct Mesh {
    // Index buffer pointing to indices in the vertex stream, to create
    // vertices.
    ID3D11Buffer* index_buffer;
    UINT triangle_count;

    // My different vertex streams
    ID3D11Buffer* vertex_streams[STREAM_COUNT];

    // AABB for the Mesh
    Math::AABB aabb;

    // -- UNUSED
    Material material;
};

// Asset Class
// Represents a renderable entity. Assets are composed of multiple
// meshes, each of which can has a material. Together, these meshes
// compose one renderable entity.
class Asset {
  private:
    std::vector<Mesh*> meshes;

    // Extra renderable properties
    // ...

  public:
    Asset();
    ~Asset();

    // Asset Creation
    void addMesh(Mesh* mesh);

    // Resource accessing
    const std::vector<Mesh*>& getMeshes() const;
    const Mesh* getMesh(int index) const;
};

} // namespace Graphics
} // namespace Engine