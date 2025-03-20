#include "GLTFFile.h"

#include <vector>

#include "MeshBuilder.h"

// The GLTFFile uses the cgltf library to read GLTF files.
// See https://github.com/jkuhlmann/cgltf
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

namespace Engine {
namespace Graphics {
GLTFFile::GLTFFile(const std::string& _path) : path(_path) {}

Asset* GLTFFile::readFromFile(MeshBuilder& builder) {
    cgltf_options options = {};
    cgltf_data* data = NULL;

    // Parse my GLTF file
    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);

    if (result != cgltf_result_success)
        return nullptr;

    result = cgltf_load_buffers(&options, data, path.c_str());

    // If success, interpret the content of the parser to read it into
    // data our engine can use
    if (result == cgltf_result_success) {
        Asset* asset = new Asset();

        std::vector<MeshTriangle> triangle_data;
        std::vector<MeshVertex> vertex_data;
        Material material;

        assert(data->meshes_count > 0);

        const uint32_t num_meshes = data->meshes_count;
        for (int i = 0; i < num_meshes; i++) {
            builder.reset();

            triangle_data.clear();
            vertex_data.clear();

            const cgltf_mesh mesh = data->meshes[i];

            // For now, we'll assume only 1 primitive list. Most meshes
            // will only have one primitive anwyays.
            assert(mesh.primitives_count == 1);
            const cgltf_primitive prim = mesh.primitives[0];

            // Parse the attributes for my primitive.
            for (int i = 0; i < prim.attributes_count; i++) {
                const cgltf_attribute attr = prim.attributes[i];
                const cgltf_attribute_type type = attr.type;

                switch (type) {
                case cgltf_attribute_type_position:
                    parsePositions(attr.data, vertex_data);
                    break;

                case cgltf_attribute_type_normal:
                    parseNormals(attr.data, vertex_data);
                    break;

                case cgltf_attribute_type_texcoord:
                    parseTextureCoord(attr.data, vertex_data);
                    break;

                default:
                    assert(false);
                    break;
                }
            }

            // Add the vertices to my builder
            const UINT start_index = builder.addVertices(vertex_data);

            // Parse the index buffer. Use this buffer to create our mesh
            parseIndices(prim.indices, triangle_data);
            builder.addTriangles(triangle_data, start_index);

            // Parse the material information
            parseMaterial(prim.material, material);

            // Register mesh under the asset
            asset->addMesh(builder.generate(material));
        }

        // Finally, free any used memory
        cgltf_free(data);

        return asset;
    } else
        return nullptr;
}

// --- Parsing ---
// Read the CGLTF data to extract information relevant to this engine.

void GLTFFile::parseIndices(const cgltf_accessor* accessor,
                            std::vector<MeshTriangle>& triangles) {
    assert(accessor->type == cgltf_type_scalar);

    const cgltf_buffer_view* view = accessor->buffer_view;
    const uint8_t* buffer = ((uint8_t*)view->buffer->data) + view->offset;

    const uint32_t num_elements = accessor->count;
    const uint32_t element_size = accessor->stride;
    assert(element_size == 2);

    uint16_t i0, i1, i2;
    for (int i = 0; i < num_elements; i += 3) {
        memcpy(&i0, buffer + element_size * i, element_size);
        memcpy(&i1, buffer + element_size * (i + 1), element_size);
        memcpy(&i2, buffer + element_size * (i + 2), element_size);

        triangles.push_back(MeshTriangle(i0, i1, i2));
    }
}

void GLTFFile::parsePositions(const cgltf_accessor* accessor,
                              std::vector<MeshVertex>& vertex_data) {
    assert(accessor->type == cgltf_type_vec3);

    const cgltf_buffer_view* view = accessor->buffer_view;
    const uint8_t* buffer = ((uint8_t*)view->buffer->data) + view->offset;

    const uint32_t num_elements = accessor->count;
    const uint32_t element_size = accessor->stride;
    const uint32_t scalar_size = element_size / 3;

    Vector3 position;
    for (int i = 0; i < num_elements; i++) {
        memcpy(&position.x, buffer + element_size * i, scalar_size);
        memcpy(&position.y, buffer + element_size * i + scalar_size,
               scalar_size);
        memcpy(&position.z, buffer + element_size * i + scalar_size * 2,
               scalar_size);

        MeshVertex& vertex = createVertexAtIndex(i, vertex_data);
        vertex.position = position;
    }
}

void GLTFFile::parseNormals(const cgltf_accessor* accessor,
                            std::vector<MeshVertex>& vertex_data) {
    assert(accessor->type == cgltf_type_vec3);

    const cgltf_buffer_view* view = accessor->buffer_view;
    const uint8_t* buffer = ((uint8_t*)view->buffer->data) + view->offset;

    const uint32_t num_elements = accessor->count;
    const uint32_t element_size = accessor->stride;
    const uint32_t scalar_size = element_size / 3;

    Vector3 normal;
    for (int i = 0; i < num_elements; i++) {
        memcpy(&normal.x, buffer + element_size * i, scalar_size);
        memcpy(&normal.y, buffer + element_size * i + scalar_size, scalar_size);
        memcpy(&normal.z, buffer + element_size * i + scalar_size * 2,
               scalar_size);

        MeshVertex& vertex = createVertexAtIndex(i, vertex_data);
        vertex.normal = normal;

        vertex.color = Color(normal.x, normal.y, normal.z);
    }
}

void GLTFFile::parseTextureCoord(const cgltf_accessor* accessor,
                                 std::vector<MeshVertex>& vertex_data) {
    assert(accessor->type == cgltf_type_vec2);

    const cgltf_buffer_view* view = accessor->buffer_view;
    const uint8_t* buffer = ((uint8_t*)view->buffer->data) + view->offset;

    const uint32_t num_elements = accessor->count;
    const uint32_t element_size = accessor->stride;
    const uint32_t scalar_size = element_size / 2;

    Vector2 tex;
    for (int i = 0; i < num_elements; i++) {
        memcpy(&tex.x, buffer + element_size * i, scalar_size);
        memcpy(&tex.y, buffer + element_size * i + scalar_size, scalar_size);

        MeshVertex& vertex = createVertexAtIndex(i, vertex_data);
        vertex.tex = tex;
    }
}

MeshVertex&
GLTFFile::createVertexAtIndex(int index, std::vector<MeshVertex>& vertex_data) {
    if (vertex_data.size() <= index)
        vertex_data.resize(index + 1);
    return vertex_data[index];
}

void GLTFFile::parseMaterial(const cgltf_material* mat_data,
                             Material& material) {
    if (mat_data->has_pbr_metallic_roughness) {
        const cgltf_pbr_metallic_roughness roughness =
            mat_data->pbr_metallic_roughness;

        material.base_color = Color(roughness.base_color_factor[0],
                                    roughness.base_color_factor[1],
                                    roughness.base_color_factor[2]);
        material.diffuse_factor = roughness.roughness_factor;
    }
}

} // namespace Graphics
} // namespace Engine