#pragma once

#include <vector>

#include "Asset.h"

namespace Engine {
namespace Graphics {
// MeshBuilder Class:
// Enables creation of meshes. Meshes are represented by a vertex and index
// buffer. The vertex buffer stores all vertices in the mesh, and the index
// buffer references these vertices by index to create triangles for the mesh.
struct MeshVertex {
    Vector3 position;
    Vector2 textureCoord;
    Vector3 normal;

    MeshVertex();
    MeshVertex(const MeshVertex& vertex);
    MeshVertex(const Vector3& pos, const Vector2& tex,
               const Math::Vector3& norm);
};

struct MeshTriangle {
    UINT vertex0;
    UINT vertex1;
    UINT vertex2;

    MeshTriangle(UINT v0, UINT v1, UINT v2);
};

class MeshBuilder {
  private:
    // Device interface for creating GPU resources
    ID3D11Device* device;

    std::vector<MeshVertex> vertex_buffer;
    std::vector<MeshTriangle> index_buffer;

  public:
    MeshBuilder(ID3D11Device* device);
    ~MeshBuilder();

    // Generates the Mesh for use in the rendering pipeline
    Mesh* generate();

    // Add a vertex to the builder, and returns the index corresponding to
    // this vertex.
    UINT addVertex(const Vector3& pos, const Vector2& tex, const Vector3& norm);

    // Add a triangle to the builder, based on indices.
    void addTriangle(UINT v1, UINT v2, UINT v3);

    // Discard the current normals for the mesh and regenerate them
    void regenerateNormals();

    // Resets the builder, so it can be used to generate another mesh
    void reset();

  private: 
    ID3D11Buffer* createVertexStream(void (*data_parser)(const MeshVertex&, uint8_t *output), UINT element_size);

    static void ExtractVertexPosition(const MeshVertex& vertex, uint8_t*output);
    static void ExtractVertexTexture(const MeshVertex& vertex, uint8_t* output);
    static void ExtractVertexNormal(const MeshVertex& vertex, uint8_t* output);
};

// TextureBuilder Class:
// Provides an interface for creating Textures.
// Only supports R8G8B8A8 textures.
struct TextureColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

class TextureBuilder {
  private:
    // Device interface for creating GPU resources
    ID3D11Device* device;

    unsigned int pixel_width;
    unsigned int pixel_height;

    std::vector<TextureColor> data;

  public:
    TextureBuilder(ID3D11Device* device, UINT _width, UINT _height);
    ~TextureBuilder();

    // Generates the renderable texture
    Texture* generate();

    // Sets the color for a particular pixel
    void setColor(UINT x, UINT y, const TextureColor& rgba);

    // Clears the texture with an rgba color
    void clear(const TextureColor& rgba);

    // Resets the builder
    void reset(unsigned int width, unsigned int height);
};
} // namespace Graphics
} // namespace Engine