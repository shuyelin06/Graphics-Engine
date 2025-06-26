#include "AssetBuilder.h"

#include <assert.h>

#include "math/Compute.h"

// Hash Function for Vector3.
// Used if vertices are to be shared.
template <> struct std::hash<Engine::Math::Vector3> {
    std::size_t operator()(const Engine::Math::Vector3& k) const {
        // https://stackoverflow.com/questions/5928725/hashing-2d-3d-and-nd-vectors
        uint32_t hash = std::_Bit_cast<uint32_t, float>(k.x) * 73856093 ^
                        std::_Bit_cast<uint32_t, float>(k.y) * 19349663 ^
                        std::_Bit_cast<uint32_t, float>(k.z) * 83492791;

        return hash % SIZE_MAX;
    }
};

namespace Engine {
namespace Graphics {
MeshVertex::MeshVertex() {
    position = Vector3(0, 0, 0);
    tex = Vector2(0.5, 0.5f);
    normal = Vector3(0, 0, 0);
    color = Color::White();
    // Debug_Line unavailable
    joints = Vector4();
    weights = Vector4();
}
MeshVertex::MeshVertex(const Vector3& _position, const Color& _color) {
    position = _position;
    color = _color;
}
MeshVertex::MeshVertex(const MeshVertex& vertex) {
    position = vertex.position;

    tex = vertex.tex;
    normal = vertex.normal;
    color = vertex.color;

    joints = vertex.joints;
    weights = vertex.weights;
}

void* MeshVertex::AddressPosition(MeshVertex& vertex) {
    return &vertex.position;
}
void* MeshVertex::AddressNormal(MeshVertex& vertex) { return &vertex.normal; }
void* MeshVertex::AddressTexture(MeshVertex& vertex) { return &vertex.tex; }
void* MeshVertex::AddressColor(MeshVertex& vertex) { return &vertex.color; }
void* MeshVertex::AddressJoints(MeshVertex& vertex) { return &vertex.joints; }
void* MeshVertex::AddressWeights(MeshVertex& vertex) { return &vertex.weights; }

MeshTriangle::MeshTriangle() { vertex0 = vertex1 = vertex2 = 0; }
MeshTriangle::MeshTriangle(UINT v0, UINT v1, UINT v2) {
    vertex0 = v0;
    vertex1 = v1;
    vertex2 = v2;
}

MeshBuilder::MeshBuilder() {
    layout = 0;
    active_color = Color::White();
}
MeshBuilder::~MeshBuilder() = default;

// Generate:
// Generates the index and vertex buffer resources for the mesh.
// With only the device, the builder will generate an independent
// MeshPool that cannot be mapped.
Mesh* MeshBuilder::generateMesh(ID3D11Device* device) {
    return generateMesh(device, Material());
}

Mesh* MeshBuilder::generateMesh(ID3D11Device* device,
                                const Material& material) {
    if (index_buffer.size() == 0 || vertex_buffer.size() == 0)
        return nullptr;

    // Create my pool
    MeshPool* pool = new MeshPool();
    pool->layout = layout;
    pool->vertex_size = pool->vertex_capacity = vertex_buffer.size();
    pool->triangle_size = pool->triangle_capacity = index_buffer.size();

    // Create index buffer
    D3D11_BUFFER_DESC buff_desc = {};
    D3D11_SUBRESOURCE_DATA sr_data = {0};

    buff_desc.ByteWidth = sizeof(MeshTriangle) * index_buffer.size();
    buff_desc.Usage = D3D11_USAGE_DEFAULT;
    buff_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    sr_data.pSysMem = (void*)index_buffer.data();

    device->CreateBuffer(&buff_desc, &sr_data, &(pool->ibuffer));
    assert(pool->ibuffer != nullptr);

    // Create each of my vertex streams
    if (LayoutPinHas(layout, POSITION))
        pool->vbuffers[POSITION] = createVertexStream(
            MeshVertex::AddressPosition, sizeof(Vector3), device);
    if (LayoutPinHas(layout, TEXTURE))
        pool->vbuffers[TEXTURE] = createVertexStream(MeshVertex::AddressTexture,
                                                     sizeof(Vector2), device);
    if (LayoutPinHas(layout, NORMAL))
        pool->vbuffers[NORMAL] = createVertexStream(MeshVertex::AddressNormal,
                                                    sizeof(Vector3), device);
    if (LayoutPinHas(layout, COLOR))
        pool->vbuffers[COLOR] =
            createVertexStream(MeshVertex::AddressColor, sizeof(Color), device);
    if (LayoutPinHas(layout, JOINTS))
        pool->vbuffers[JOINTS] = createVertexStream(MeshVertex::AddressJoints,
                                                    sizeof(Vector4), device);
    if (LayoutPinHas(layout, WEIGHTS))
        pool->vbuffers[WEIGHTS] = createVertexStream(MeshVertex::AddressWeights,
                                                     sizeof(Vector4), device);

    // Create my mesh
    Mesh* mesh = new Mesh();
    mesh->layout = layout;
    mesh->buffer_pool = pool;
    mesh->vertex_start = 0;
    mesh->num_vertices = vertex_buffer.size();
    mesh->triangle_start = 0;
    mesh->num_triangles = index_buffer.size();

    for (const MeshVertex& vertex : vertex_buffer)
        mesh->aabb.expandToContain(vertex.position);

    mesh->material = material;

    return mesh;
}

// CreateVertexStream:
// Creates a vertex stream for some data extracted from the MeshVertex.
// Builds this by using a function that, given the MeshVertex, writes
// element_size bytes of data to output (assumed to be the same size). This
// function could extract position, normals, or a combination of data.
ID3D11Buffer* MeshBuilder::createVertexStream(void* (*addressor)(MeshVertex&),
                                              UINT byte_size,
                                              ID3D11Device* device) {
    const UINT NUM_VERTICES = vertex_buffer.size();

    // Iterate through each vertex, and extract the data from it.
    // Write this data to a vector which we will use to generate our vertex
    // stream.
    std::vector<uint8_t> stream_data;
    stream_data.resize(NUM_VERTICES * byte_size);

    for (int i = 0; i < vertex_buffer.size(); i++) {
        // Copy my data to the stream
        void* address = (*addressor)(vertex_buffer[i]);
        memcpy(&stream_data[i * byte_size], address, byte_size);
    }

    // Generate a buffer for this data stream
    ID3D11Buffer* buffer_handle = NULL;

    D3D11_BUFFER_DESC buff_desc = {};
    D3D11_SUBRESOURCE_DATA sr_data = {0};

    buff_desc.ByteWidth = NUM_VERTICES * byte_size;
    buff_desc.Usage = D3D11_USAGE_DEFAULT;
    buff_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    sr_data.pSysMem = (void*)stream_data.data();

    device->CreateBuffer(&buff_desc, &sr_data, &buffer_handle);
    assert(buffer_handle != nullptr);

    return buffer_handle;
}

Mesh* MeshBuilder::generateMesh(ID3D11DeviceContext* context, MeshPool* pool) {
    return generateMesh(context, pool, Material());
}

Mesh* MeshBuilder::generateMesh(ID3D11DeviceContext* context, MeshPool* pool,
                                const Material& material) {
    // Layout must match the pool's layout
    assert((layout & pool->layout) == layout);
    // Pool must have enough space for this mesh
    assert(vertex_buffer.size() + pool->vertex_size <= pool->vertex_capacity);
    assert(index_buffer.size() + pool->triangle_size <=
           pool->triangle_capacity);
    // Pool must be mappable
    assert(pool->mappable);

    // TODO: If the pool does not work for the mesh, we can consider
    // calling the other generation method instead of failing...
    
    // Upload my index buffer data
    D3D11_MAPPED_SUBRESOURCE i_sr = {0};
    context->Map(pool->ibuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &i_sr);

    uint8_t* write_addr = static_cast<uint8_t*>(i_sr.pData) +
                          pool->triangle_size * sizeof(MeshTriangle);
    memcpy(write_addr, index_buffer.data(),
           index_buffer.size() * sizeof(MeshTriangle));

    context->Unmap(pool->ibuffer, 0);

    // Upload my vertex buffer data. We have to allocate based on pool's layout
    // to keep the vertices aligned. This means that space could be wasted if
    // the pool supports streams that the builder does not have.
    if (LayoutPinHas(pool->layout, POSITION))
        uploadVertexData(context, pool->vbuffers[POSITION], pool->vertex_size,
                         MeshVertex::AddressPosition, sizeof(Vector3));
    if (LayoutPinHas(pool->layout, TEXTURE))
        uploadVertexData(context, pool->vbuffers[TEXTURE], pool->vertex_size,
                         MeshVertex::AddressTexture, sizeof(Vector2));
    if (LayoutPinHas(pool->layout, NORMAL))
        uploadVertexData(context, pool->vbuffers[NORMAL], pool->vertex_size,
                         MeshVertex::AddressNormal, sizeof(Vector3));
    if (LayoutPinHas(pool->layout, COLOR))
        uploadVertexData(context, pool->vbuffers[COLOR], pool->vertex_size,
                         MeshVertex::AddressColor, sizeof(Vector3));
    if (LayoutPinHas(pool->layout, JOINTS))
        uploadVertexData(context, pool->vbuffers[JOINTS], pool->vertex_size,
                         MeshVertex::AddressJoints, sizeof(Vector4));
    if (LayoutPinHas(pool->layout, WEIGHTS))
        uploadVertexData(context, pool->vbuffers[WEIGHTS], pool->vertex_size,
                         MeshVertex::AddressWeights, sizeof(Vector4));

    // Create my mesh
    Mesh* mesh = new Mesh();
    mesh->layout = layout;
    mesh->buffer_pool = pool;
    mesh->is_pool_mine = true;
    mesh->vertex_start = pool->vertex_size;
    mesh->num_vertices = vertex_buffer.size();
    mesh->triangle_start = pool->triangle_size;
    mesh->num_triangles = index_buffer.size();

    for (const MeshVertex& vertex : vertex_buffer)
        mesh->aabb.expandToContain(vertex.position);

    mesh->material = material;

    // Update my mesh pool
    pool->vertex_size += vertex_buffer.size();
    pool->triangle_size += index_buffer.size();

    return mesh;
}

// UploadBufferData:
// Given a buffer, uploads vertex data to that buffer, starting
// buffer_start bytes into the buffer.
void MeshBuilder::uploadVertexData(ID3D11DeviceContext* context,
                                   ID3D11Buffer* buffer, UINT buffer_size,
                                   void* (*addressor)(MeshVertex&),
                                   UINT byte_size) {
    // First, map my buffer to obtain it CPU side and find where I will begin
    // writing to.
    D3D11_MAPPED_SUBRESOURCE sr = {0};
    context->Map(buffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &sr);
    uint8_t* write_addr =
        static_cast<uint8_t*>(sr.pData) + buffer_size * byte_size;

    // Now, for each vertex, I will pull the data I want for my stream and
    // then copy it to the end of my buffer.
    for (int i = 0; i < vertex_buffer.size(); i++) {
        void* address = (*addressor)(vertex_buffer[i]);
        memcpy(write_addr + i * byte_size, address, byte_size);
    }

    // Finally, return the buffer data to the GPU
    context->Unmap(buffer, 0);
}

const std::vector<MeshVertex>& MeshBuilder::getVertices() const {
    return vertex_buffer;
}
const std::vector<MeshTriangle>& MeshBuilder::getIndices() const {
    return index_buffer;
}

// SetColor:
// Sets the active color
void MeshBuilder::setColor(const Color& color) { active_color = color; }

// AddLayout:
// Add a layout vertex buffer for the builder to generate with
void MeshBuilder::addLayout(VertexDataStream stream) {
    layout = layout | (1 << stream);
}

// AddVertex:
// Adds a vertex with position, texture, and norm to the MeshBuilder.
UINT MeshBuilder::addVertex(const MeshVertex& vertex) {
    UINT index = vertex_buffer.size();
    vertex_buffer.push_back(vertex);
    return index;
}

UINT MeshBuilder::addVertex(const Vector3& pos) {
    UINT index = vertex_buffer.size();
    vertex_buffer.push_back(MeshVertex(pos, active_color));
    return index;
}

UINT MeshBuilder::addVertices(const std::vector<MeshVertex>& vertices) {
    UINT start_index = vertex_buffer.size();
    for (const MeshVertex& vertex : vertices)
        addVertex(vertex);
    return start_index;
}

// AddTriangle:
// Adds a triangle to the MeshBuilder with indices specified by the parameters.
void MeshBuilder::addTriangle(UINT v1, UINT v2, UINT v3) {
    index_buffer.push_back(MeshTriangle(v1, v2, v3));
}

void MeshBuilder::addTriangles(const std::vector<MeshTriangle>& indices,
                               UINT start_index) {
    for (const MeshTriangle& triangle : indices) {
        addTriangle(triangle.vertex0 + start_index,
                    triangle.vertex1 + start_index,
                    triangle.vertex2 + start_index);
    }
}

void MeshBuilder::popTriangles(UINT num_triangles) {
    for (int i = 0; i < num_triangles; i++) {
        if (index_buffer.size() > 0)
            index_buffer.pop_back();
    }
}

// GetVertex;
// Return a MeshVertex from the builder (by index) for modification
MeshVertex& MeshBuilder::getVertex(UINT index) { return vertex_buffer[index]; }
Vector3& MeshBuilder::getPosition(UINT index) {
    return vertex_buffer[index].position;
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
    // Only do this if the builder is to generate the normal stream
    assert(LayoutPinHas(layout, NORMAL));

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

    layout = 0;
}

} // namespace Graphics
} // namespace Engine