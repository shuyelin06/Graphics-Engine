#pragma once

#include <string>
#include <vector>

#include "../Direct3D11.h"
#include "VertexStreamIDs.h"
#include "Texture.h"

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
    Color ka; // Ambient Color
    Color kd; // Diffuse Color
    Color ks; // Specular Color

    std::string texture; // Texture

    Material();
};



// Struct Mesh:
// Specifies a mesh, which is a collection of vertices that form triangles. 
// Vertices are stored in separate vertex streams, so that they have an easier time being
// passed as input into shaders. 
struct Mesh {
    // Index buffer pointing to indices in the vertex stream, to create vertices.
    ID3D11Buffer* index_buffer;
    UINT triangle_count;

    // My different vertex streams
    ID3D11Buffer* vertex_streams[STREAM_COUNT];

    Material* material;   
};

// Asset Class
// Represents a renderable entity. Assets are composed of multiple
// meshes, each of which can has a material. Together, these meshes
// compose one renderable entity.
class Asset {
  private:
    Mesh* mesh;

    // Extra renderable properties
    // ...

  public:
    Asset(Mesh* mesh);
    ~Asset();

    // Resource accessing
    const Mesh* getMesh() const;
};

} // namespace Graphics
} // namespace Engine