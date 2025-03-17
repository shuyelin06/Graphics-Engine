#include "MeshBuilder.h"

#include <assert.h>

#include "math/Compute.h"

namespace Engine {
namespace Graphics {
MeshVertex::MeshVertex() = default;
MeshVertex::MeshVertex(const Vector3& _position, const Color& _color) {
    position = _position;
    color = _color;
}
MeshVertex::MeshVertex(const MeshVertex& vertex) {
    position = vertex.position;

    tex = vertex.tex;
    normal = vertex.normal;
    color = vertex.color;
}

MeshTriangle::MeshTriangle(UINT v0, UINT v1, UINT v2) {
    vertex0 = v0;
    vertex1 = v1;
    vertex2 = v2;
}

MeshBuilder::MeshBuilder(ID3D11Device* _device) : device(_device) {
    active_color = Color::White();
};

MeshBuilder::~MeshBuilder() = default;

// Generate:
// Generates the index and vertex buffer resources for the mesh.
Mesh* MeshBuilder::generate() {
    if (index_buffer.size() == 0 || vertex_buffer.size() == 0)
        return nullptr;

    Mesh* mesh = new Mesh();
    mesh->triangle_count = index_buffer.size();

    // Create index buffer
    D3D11_BUFFER_DESC buff_desc = {};
    D3D11_SUBRESOURCE_DATA sr_data = {0};

    buff_desc.ByteWidth = sizeof(MeshTriangle) * index_buffer.size();
    buff_desc.Usage = D3D11_USAGE_DEFAULT;
    buff_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    sr_data.pSysMem = (void*)index_buffer.data();

    device->CreateBuffer(&buff_desc, &sr_data, &(mesh->index_buffer));
    assert(mesh->index_buffer != nullptr);

    // Create each of my vertex streams
    mesh->vertex_streams[POSITION] =
        createVertexStream(ExtractVertexPosition, sizeof(float) * 3);
    mesh->vertex_streams[TEXTURE] =
        createVertexStream(ExtractVertexTexture, sizeof(float) * 2);
    mesh->vertex_streams[NORMAL] =
        createVertexStream(ExtractVertexNormal, sizeof(float) * 3);
    mesh->vertex_streams[COLOR] =
        createVertexStream(ExtractVertexColor, sizeof(float) * 3);

    // Generate my AABB extents
    for (const MeshVertex& vertex : vertex_buffer)
        mesh->aabb.expandToContain(vertex.position);

    return mesh;
}

// CreateVertexStream:
// Creates a vertex stream for some data extracted from the MeshVertex.
// Builds this by using a function that, given the MeshVertex, writes
// element_size bytes of data to output (assumed to be the same size). This
// function could extract position, normals, or a combination of data.
ID3D11Buffer* MeshBuilder::createVertexStream(
    void (*data_parser)(const MeshVertex& vertex, uint8_t* output),
    UINT element_size) {
    const UINT NUM_VERTICES = vertex_buffer.size();

    // Will store my per-vertex data
    uint8_t* data_element = new uint8_t[element_size];

    // Iterate through each vertex, and extract the data from it.
    // Write this data to a vector which we will use to generate our vertex
    // stream.
    std::vector<uint8_t> stream_data;
    stream_data.reserve(NUM_VERTICES * element_size);

    for (int i = 0; i < vertex_buffer.size(); i++) {
        // Pull my vertex data
        (*data_parser)(vertex_buffer[i], data_element);

        // Add my vertex data to the stream
        for (int j = 0; j < element_size; j++)
            stream_data.push_back(data_element[j]);
    }

    // Generate a buffer for this data stream
    ID3D11Buffer* buffer_handle = NULL;

    D3D11_BUFFER_DESC buff_desc = {};
    D3D11_SUBRESOURCE_DATA sr_data = {0};

    buff_desc.ByteWidth = NUM_VERTICES * element_size;
    buff_desc.Usage = D3D11_USAGE_DEFAULT;
    buff_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    sr_data.pSysMem = (void*)stream_data.data();

    device->CreateBuffer(&buff_desc, &sr_data, &buffer_handle);
    assert(buffer_handle != nullptr);

    delete[] data_element;

    return buffer_handle;
}

// Extraction Methods:
// Builds a corresponding vertex stream by extracting data from vertex and
// writing it to the output data array. Vertex position is 3 floats (x,y,z), 4
// bytes each.
void MeshBuilder::ExtractVertexPosition(const MeshVertex& vertex,
                                        uint8_t* output) {
    memcpy(output, &vertex.position, 3 * sizeof(float));
}

void MeshBuilder::ExtractVertexTexture(const MeshVertex& vertex,
                                       uint8_t* output) {
    memcpy(output, &vertex.tex, 2 * sizeof(float));
}

void MeshBuilder::ExtractVertexNormal(const MeshVertex& vertex,
                                      uint8_t* output) {
    memcpy(output, &vertex.normal, 3 * sizeof(float));
}

void MeshBuilder::ExtractVertexColor(const MeshVertex& vertex,
                                     uint8_t* output) {
    memcpy(output, &vertex.color, 3 * sizeof(float));
}

// SetColor:
// Sets the active color
void MeshBuilder::setColor(const Color& color) { active_color = color; }

// AddVertex:
// Adds a vertex with position, texture, and norm to the MeshBuilder.
UINT MeshBuilder::addVertex(const Vector3& pos) {
    UINT index = vertex_buffer.size();
    vertex_buffer.push_back(MeshVertex(pos, active_color));
    return index;
}

// AddTriangle:
// Adds a triangle to the MeshBuilder with indices specified by the parameters.
void MeshBuilder::addTriangle(UINT v1, UINT v2, UINT v3) {
    index_buffer.push_back(MeshTriangle(v1, v2, v3));
}

// AddShapes:
// Generates various shapes (given parameters) and adds them to the mesh
// builder.
// Creates a plane with boundaries given as vertices a,b,c,d in CCW order.
void MeshBuilder::addTriangle(const Vector3& a, const Vector3& b,
                              const Vector3& c) {
    const int i0 = addVertex(a);
    const int i1 = addVertex(b);
    const int i2 = addVertex(c);

    addTriangle(i0, i1, i2);
}

void MeshBuilder::addCube(const Vector3& center, const Quaternion& rotation,
                          float size) {
    // Cube vertices and indices
    Vector3 vertices[] = {
        Vector3(0.5f, -0.5f, 0.5f),   Vector3(0.5f, -0.5f, -0.5f),
        Vector3(-0.5f, -0.5f, -0.5f), Vector3(-0.5f, -0.5f, 0.5f),
        Vector3(0.5f, 0.5f, 0.5f),    Vector3(0.5f, 0.5f, -0.5f),
        Vector3(-0.5f, 0.5f, -0.5f),  Vector3(-0.5f, 0.5f, 0.5f)};

    const int indices[] = {// Bottom
                           0, 3, 2, 1,
                           // Top
                           4, 5, 6, 7,
                           // Front
                           0, 4, 7, 3,
                           // Right
                           0, 1, 5, 4,
                           // Back
                           2, 6, 5, 1,
                           // Left
                           3, 7, 6, 2};

    // Transform my vertices.
    const Matrix3 m_rotation = rotation.rotationMatrix3();

    for (int i = 0; i < 8; i++) {
        vertices[i] = center + m_rotation * (vertices[i] * size);
    }

    // Add the faces of the cube. We need to add repeat vertices so that the
    // normals for each face are sharp.
    for (int i = 0; i < 24; i += 4) {
        const int i0 = addVertex(vertices[indices[i]]);
        const int i1 = addVertex(vertices[indices[i + 1]]);
        const int i2 = addVertex(vertices[indices[i + 2]]);
        const int i3 = addVertex(vertices[indices[i + 3]]);

        addTriangle(i0, i1, i2);
        addTriangle(i2, i3, i0);
    }
}

// AddTube:
// Creates a tube between start and end.
void MeshBuilder::addTube(const Vector3& start, const Vector3& end,
                          float radius, int num_vertices) {
    assert(num_vertices >= 3);

    // Find the vector from the start to the end. Based on this, we'll create 2
    // perpendicular vectors that'll create a plane. We'll use this plane to
    // generate our points.
    const Vector3 direction = (end - start).unit();

    const Vector3 perp_x = direction.orthogonal().unit();
    const Vector3 perp_y = direction.cross(perp_x).unit();

    // Now, generate the points at each cap of this tube
    const int start_index = addVertex(start);
    for (int i = 0; i < num_vertices; i++) { // Bottom of the tube
        const float angle = 2 * PI / num_vertices * i;

        const float x = cosf(angle);
        const float y = sinf(angle);

        const Vector3 point = start + perp_x * x + perp_y * y;
        addVertex(point);
    }

    const int end_index = addVertex(end);
    for (int i = 0; i < num_vertices; i++) { // Top of the tube
        const float angle = 2 * PI / num_vertices * i;

        const float x = cosf(angle);
        const float y = sinf(angle);

        const Vector3 point = end + perp_x * x + perp_y * y;
        addVertex(point);
    }

    // Now, connect the points.
    for (int i = 1; i <= num_vertices; i++) {
        const int bottom_i1 = start_index + i;
        const int bottom_i2 =
            (i != num_vertices) ? bottom_i1 + 1 : start_index + 1;
        const int top_i1 = end_index + i;
        const int top_i2 = (i != num_vertices) ? top_i1 + 1 : end_index + 1;

        // Connect the points to form the shaft
        addTriangle(bottom_i1, bottom_i2, top_i1);
        addTriangle(bottom_i2, top_i2, top_i1);

        // Connect the points to form the cap
        addTriangle(start_index, bottom_i2, bottom_i1);
        addTriangle(end_index, top_i2, top_i1);
    }
}

// RegenerateNormals:
// Discard the current normals for the mesh and regenerate them
void MeshBuilder::regenerateNormals() {
    // Regenerate mesh normals. We do this by calculating the normal
    // for each triangle face, and adding them to a vector to
    // accumulate their contribution to each vertex
    std::vector<Vector3> meshNormals;
    meshNormals.resize(vertex_buffer.size());

    for (int i = 0; i < index_buffer.size(); i++) {
        // Calculate vertex normal
        const MeshTriangle& triangle = index_buffer[i];

        const Vector3& vertex0 = vertex_buffer[triangle.vertex0].position;
        const Vector3& vertex1 = vertex_buffer[triangle.vertex1].position;
        const Vector3& vertex2 = vertex_buffer[triangle.vertex2].position;

        const Vector3 normal = (vertex1 - vertex0).cross(vertex2 - vertex0);

        // Add this normal's contribution for all vertices of the face
        meshNormals[triangle.vertex0] += normal;
        meshNormals[triangle.vertex1] += normal;
        meshNormals[triangle.vertex2] += normal;
    }

    // Iterate through all vertices in the mesh. If their normal is degenerate
    // (0,0,0), replace it with the generated normal.
    for (int i = 0; i < vertex_buffer.size(); i++) {
        Vector3& normal = vertex_buffer[i].normal;

        if (normal.magnitude() == 0) {
            meshNormals[i].inplaceNormalize();
            vertex_buffer[i].normal = meshNormals[i];
        }
    }
}

// Reset:
// Clears the MeshBuilder so it can be used to generate another mesh
void MeshBuilder::reset() {
    vertex_buffer.clear();
    index_buffer.clear();
}

} // namespace Graphics
} // namespace Engine