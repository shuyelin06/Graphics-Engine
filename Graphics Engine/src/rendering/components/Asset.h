#pragma once

#include "rendering/Direct3D11.h"

#include <string>
#include <vector>

#include "math/Color.h"
#include "math/Vector2.h"
#include "math/Vector3.h"

namespace Engine {
using namespace Math;
namespace Graphics {
typedef unsigned int UINT;

// Struct Texture:
// Specifies a 2D array of data for a mesh.
struct Texture {
    ID3D11Texture2D* texture;

    ID3D11ShaderResourceView* view;
    ID3D11SamplerState* sampler;

    UINT width;
    UINT height;
};

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
// Specifies a mesh, which is a collection of vertices
// which from triangles. Each vertex has a
// 1) Position
// 2) Texture Coordinate
// 3) Normal
struct Mesh {
    ID3D11Buffer* index_buffer;
    ID3D11Buffer* vertex_buffer;

    // Buffer containing only vertex information, for use in the shadowmap
    // pass.
    ID3D11Buffer* shadowmap_buffer; 
    
    Material* material;
    UINT triangle_count;
};

// Asset Class
// Represents a renderable entity. Assets are composed of multiple
// meshes, each of which can has a material. Together, these meshes
// compose one renderable entity.
class Asset {
  private:
    std::vector<Mesh*> meshes;
    std::vector<Material*> materials;

  public:
    Asset();
    ~Asset();

    // Resource creation
    void addMesh(Mesh* mesh);
    void addMaterial(Material* material);

    // Resource accessing
    std::vector<Mesh*>& getMeshes();
    std::vector<Material*>& getMaterials();

    Mesh* getMesh(int mesh_index);
    Material* getMaterial(int material_index);
};

} // namespace Graphics
} // namespace Engine