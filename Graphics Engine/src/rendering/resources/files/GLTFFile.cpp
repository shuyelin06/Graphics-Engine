#include "GLTFFile.h"

#include <assert.h>
#include <functional>
#include <unordered_map>
#include <vector>

#include <string>

// The GLTFFile uses the cgltf library to read GLTF files.
// See https://github.com/jkuhlmann/cgltf
#include "cgltf/cgltf.h"

// The GLTFFile uses the STB library to read image files
#include "stb/stb_image.h"

namespace Engine {
namespace Graphics {
GLTFFile::GLTFFile(const std::string& _path) : path(_path) {}

struct Uint4 {
    uint16_t index[4];
};
// Accessor Parsing functions that let us convert accessor data into a
// format the engine uses.
static void
ParseAccessor(const cgltf_accessor* accessor,
              const std::function<void(int, const void*, size_t)>& callback) {
    // f is invoked for every parsed element from the buffer. We can use that to
    // read the data wherever we want.
    // f(index, elementData, elementSize)
    const cgltf_buffer_view* view = accessor->buffer_view;
    const uint8_t* buffer = ((uint8_t*)view->buffer->data) + view->offset;

    const uint32_t num_elements = accessor->count;
    const uint32_t stride = accessor->stride;

    for (int i = 0; i < num_elements; i++) {
        callback(i, buffer + stride * i, stride);
    }
}

Asset* GLTFFile::readFromFile(MeshBuilder& mesh_builder,
                              AtlasBuilder& tex_builder, ID3D11Device* device,
                              ID3D11DeviceContext* context) {
    // Deprecated, to be reimplemented later.
    return nullptr;
    /*
    cgltf_options options = {};
    cgltf_data* data = NULL;

    // Parse my GLTF file
    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);

    if (result != cgltf_result_success)
        return nullptr;

    result = cgltf_load_buffers(&options, data, path.c_str());

    // If success, interpret the content of the parser to read it into
    // data our engine can use
    if (result != cgltf_result_success)
        return nullptr;

    Asset* asset = new Asset();

    // Parse my meshes and materials
    {
        // Mesh Data
        std::vector<MeshTriangle> triangle_data;
        std::vector<MeshVertex> vertex_data;
        Material material;

        // Intermediate Data
        // u16_data is used to read the index buffer and convert it into a
        // format suitable for the engine.
        std::vector<uint16_t> u16_data;
        std::vector<Uint4> u4b_data;

        assert(data->meshes_count > 0);
        const uint32_t num_meshes = data->meshes_count;
        for (int i_mesh = 0; i_mesh < num_meshes; i_mesh++) {
            const cgltf_mesh mesh = data->meshes[i_mesh];

            assert(mesh.primitives_count > 0);
            const uint32_t num_prims = mesh.primitives_count;
            for (int i_prim = 0; i_prim < num_prims; i_prim++) {
                mesh_builder.reset();

                triangle_data.clear();
                vertex_data.clear();
                u16_data.clear();
                material = Material();

                const cgltf_primitive prim = mesh.primitives[i_prim];
                assert(prim.type == cgltf_primitive_type_triangles);

                // Parse the attributes for my primitive.
                for (int i_attr = 0; i_attr < prim.attributes_count; i_attr++) {
                    const cgltf_attribute attr = prim.attributes[i_attr];
                    const cgltf_attribute_type type = attr.type;

                    switch (type) {
                    case cgltf_attribute_type_position:
                        mesh_builder.addLayout(POSITION);
                        ParseVertexProperty(attr.data, POSITION,
                                            sizeof(Vector3), vertex_data);
                        break;

                    case cgltf_attribute_type_texcoord:
                        mesh_builder.addLayout(TEXTURE);
                        ParseVertexProperty(attr.data, TEXTURE, sizeof(Vector2),
                                            vertex_data);
                        break;

                    case cgltf_attribute_type_normal:
                        mesh_builder.addLayout(NORMAL);
                        ParseVertexProperty(attr.data, NORMAL, sizeof(Vector3),
                                            vertex_data);
                        break;

                    case cgltf_attribute_type_joints: {
                        mesh_builder.addLayout(JOINTS);
                        ParseAccessor<Uint4>(attr.data, u4b_data);
                        vertex_data.resize(u4b_data.size());
                        for (int i = 0; i < u4b_data.size(); i++) {
                            Vector4& joint = vertex_data[i].joints;
                            joint.x = u4b_data[i].index[0];
                            joint.y = u4b_data[i].index[1];
                            joint.z = u4b_data[i].index[2];
                            joint.w = u4b_data[i].index[3];
                        }
                    } break;

                    case cgltf_attribute_type_weights:
                        mesh_builder.addLayout(WEIGHTS);
                        ParseVertexProperty(attr.data, WEIGHTS, sizeof(Vector4),
                                            vertex_data);
                        break;

                    default:
                        assert(false);
                        break;
                    }
                }

                // Add the vertices to my builder
                const UINT start_index = mesh_builder.addVertices(vertex_data);

                // Parse the index buffer. Use this buffer to create our mesh
                ParseAccessor<uint16_t>(prim.indices, u16_data);
                for (int i = 0; i < u16_data.size(); i += 3) {
                    triangle_data.push_back(MeshTriangle(
                        u16_data[i], u16_data[i + 1], u16_data[i + 2]));
                }
                mesh_builder.addTriangles(triangle_data, start_index);

                // Parse the material information
                parseMaterial(prim.material, mesh_builder, material,
                              tex_builder);

                // Register mesh under the asset
                std::shared_ptr<Mesh> generated_mesh =
                    mesh_builder.generateMesh(context);
                asset->addMesh(generated_mesh);
            }
        }
    }

    // Parse my nodes
    std::unordered_map<std::string, Node*> node_map;

    {
        const cgltf_node* nodes = data->nodes;
        const UINT num_nodes = data->nodes_count;

        // Create my nodes and initialize their local transforms
        for (int i = 0; i < num_nodes; i++) {
            const cgltf_node node_data = nodes[i];

            // Create my node
            Node* node = new Node();

            // Set the node local transform
            if (node_data.has_translation) {
                node->transform.setPosition(node_data.translation[0],
                                            node_data.translation[1],
                                            node_data.translation[2]);
            }

            if (node_data.has_rotation) {
                node->transform.setRotation(Quaternion(
                    Vector3(node_data.rotation[0], node_data.rotation[1],
                            node_data.rotation[2]),
                    node_data.rotation[3]));
            }

            if (node_data.has_scale) {
                node->transform.setScale(node_data.scale[0], node_data.scale[1],
                                         node_data.scale[2]);
            }

            // Add my node to the asset
            asset->addNode(node);

            // Associate node with its name for later use
            node_map[node_data.name] = node;
        }

        // Iterate back through my nodes and add the node's children
        for (int i = 0; i < num_nodes; i++) {
            const cgltf_node node_data = nodes[i];
            Node* node = node_map[node_data.name];

            for (int j = 0; j < node_data.children_count; j++) {
                const cgltf_node* child_data = node_data.children[j];

                Node* child = node_map[child_data->name];
                node->children.push_back(child);

                child->parent = node;
            }
        }
    }

    // Parse my skin information
    if (data->skins_count > 0) {
        // Only works for 1 skin for now
        assert(data->skins_count == 1);

        const cgltf_skin skin = data->skins[0];

        std::vector<Matrix4> m_data;
        ParseAccessor<Matrix4>(skin.inverse_bind_matrices, m_data);
        assert(m_data.size() == skin.joints_count);

        for (int i = 0; i < skin.joints_count; i++) {
            const Node* joint_node = node_map[skin.joints[i]->name];
            const Matrix4& joint_matrix = m_data[i];

            asset->addSkinJoint(joint_node, joint_matrix);
        }
    }

    // Parse my animations (if available).
    if (data->animations_count > 0) {
        const UINT num_animations = data->animations_count;
        for (int i = 0; i < num_animations; i++) {
            const cgltf_animation anim_data = data->animations[i];

            Animation* animation = new Animation();

            // Parse each of my animation's channels
            for (int state_index = 0; state_index < anim_data.channels_count;
                 state_index++) {
                const cgltf_animation_channel channel =
                    anim_data.channels[state_index];

                // Determine the target node and the transform property this
                // state is influencing
                Node* target_node = node_map[channel.target_node->name];
                LocalStateType state_type = ANIMATION_POSITION;

                switch (channel.target_path) {
                case cgltf_animation_path_type_translation:
                    state_type = ANIMATION_POSITION;
                    break;

                case cgltf_animation_path_type_rotation:
                    state_type = ANIMATION_ROTATION;
                    break;

                case cgltf_animation_path_type_scale:
                    state_type = ANIMATION_SCALE;
                    break;

                default:
                    assert(false);
                    break;
                }

                AnimationState& state =
                    animation->newAnimationState(target_node, state_type);

                // Populate the state with data
                const cgltf_animation_sampler* sampler = channel.sampler;
                assert(sampler->input->component_type ==
                       cgltf_component_type_r_32f);
                assert(sampler->input->type == cgltf_type_scalar);
                assert(sampler->input->count == sampler->output->count);

                LocalState state_data;

                const UINT num_datapoints = sampler->input->count;

                const uint8_t* input_buffer =
                    ((uint8_t*)sampler->input->buffer_view->buffer->data) +
                    sampler->input->buffer_view->offset;
                const UINT input_size = sampler->input->stride;

                const uint8_t* output_buffer =
                    ((uint8_t*)sampler->output->buffer_view->buffer->data) +
                    sampler->output->buffer_view->offset;
                const UINT output_size = sampler->output->stride;

                for (int i_data = 0; i_data < num_datapoints; i_data++) {
                    float time;
                    memcpy(&time, input_buffer + i_data * input_size,
                           input_size);

                    Vector4 data;
                    memcpy(&data, output_buffer + i_data * output_size,
                           output_size);

                    state_data.setData(data);
                    state_data.setTime(time);

                    state.addState(state_data);
                }

                state.normalizeTimes();
            }

            asset->addAnimation(animation);
        }
    }

    // Finally, free any used memory
    cgltf_free(data);

    return asset;
    */
}

void GLTFFile::ReadGLTFMesh(const std::string& path, MeshBuilder& builder) {
    builder.reset();

    cgltf_options options = {};
    cgltf_data* data = NULL;

    // Parse my GLTF file (.gltf) and load the buffers (.bin).
    // Both files should be in the same folder, somewhere in data/
    cgltf_result result;

    result = cgltf_parse_file(&options, path.c_str(), &data);
    if (result != cgltf_result_success)
        return;

    result = cgltf_load_buffers(&options, data, path.c_str());
    if (result != cgltf_result_success)
        return;

    // The GLTF parser makes a few assumptions:
    // 1) We are only dealing with 1 mesh with 1 primitive.
    //    We could support more if needed later, but this is enough for my
    //    setup.
    // 2) This mesh can have position, normal, and UVs
    // 3) This mesh can have a material, which is processed separately.
    // TODO: Material support
    const uint32_t num_meshes = data->meshes_count;
    assert(num_meshes == 1); // We can support more
    const cgltf_mesh& mesh = data->meshes[0];

    const uint32_t num_prims = mesh.primitives_count;
    assert(mesh.primitives_count == 1);
    const cgltf_primitive& prim = mesh.primitives[0];
    assert(prim.type == cgltf_primitive_type_triangles);

    // Parse the attributes for my primitive.
    const uint32_t element_count = prim.attributes[0].data->count;

    std::vector<MeshVertex>& vertex_data = builder.getVertices();
    vertex_data.clear();
    vertex_data.resize(element_count);

    for (int i_attr = 0; i_attr < prim.attributes_count; i_attr++) {
        const cgltf_attribute& attr = prim.attributes[i_attr];
        const cgltf_attribute_type& type = attr.type;
        const auto& accessor = attr.data;
        assert(accessor->count == element_count);

        // Don't support joints and weights (for now; see deprecated
        // function for how to add these again later)
        assert(type != cgltf_attribute_type_joints);
        assert(type != cgltf_attribute_type_weights);

        switch (type) {
        case cgltf_attribute_type_position:
            builder.addLayout(POSITION);
            ParseAccessor(accessor, [&vertex_data](int index, const void* data,
                                                   size_t size) {
                assert(size == sizeof(Vector3));
                memcpy(&vertex_data[index].position, data, sizeof(Vector3));
            });
            break;

        case cgltf_attribute_type_texcoord:
            builder.addLayout(TEXTURE);
            ParseAccessor(accessor, [&vertex_data](int index, const void* data,
                                                   size_t size) {
                assert(size == sizeof(Vector2));
                memcpy(&vertex_data[index].tex, data, sizeof(Vector2));
            });
            break;

        case cgltf_attribute_type_normal:
            builder.addLayout(NORMAL);
            ParseAccessor(accessor, [&vertex_data](int index, const void* data,
                                                   size_t size) {
                assert(size == sizeof(Vector3));
                memcpy(&vertex_data[index].normal, data, sizeof(Vector3));
            });
            break;

        default:
            assert(false); // Unsupported
            break;
        }
    }

    // Parse the index buffer. Use this buffer to create our mesh
    std::vector<MeshTriangle>& triangle_data = builder.getIndices();
    triangle_data.clear();
    triangle_data.resize(prim.indices->count / 3);

    ParseAccessor(prim.indices,
                  [&triangle_data](int index, const void* data, size_t size) {
                      // TODO: This may only work for some systems due to the
                      // endianness? May need to check.
                      assert(size <= sizeof(uint32_t));
                      uint32_t* dest = (uint32_t*)triangle_data.data();
                      memcpy(dest + index, data, size);
                  });

    // Finally, free any used memory
    cgltf_free(data);
}

// --- Parsing ---
void GLTFFile::parseMaterial(const cgltf_material* mat_data,
                             MeshBuilder& builder, Material& material,
                             AtlasBuilder& tex_builder) {
    if (mat_data->has_pbr_metallic_roughness) {
        builder.addLayout(TEXTURE);

        const cgltf_pbr_metallic_roughness roughness =
            mat_data->pbr_metallic_roughness;

        // Base Color
        AtlasAllocation alloc;

        if (roughness.base_color_texture.texture != nullptr) {
            alloc = parseBaseColorTex(roughness.base_color_texture.texture,
                                      tex_builder);
        } else {
            alloc = tex_builder.allocateRegion(1, 1);

            TextureColor color;
            color.r = 255 * roughness.base_color_factor[0];
            color.g = 255 * roughness.base_color_factor[1];
            color.b = 255 * roughness.base_color_factor[2];
            color.a = 255;

            tex_builder.setColor(0, 0, color);
        }

        TextureRegion region;
        region.x = float(alloc.x) / tex_builder.getAtlasWidth();
        region.y = float(alloc.y) / tex_builder.getAtlasHeight();
        region.width = float(alloc.width) / tex_builder.getAtlasWidth();
        region.height = float(alloc.height) / tex_builder.getAtlasHeight();

        material.tex_region = region;

        // Diffuse Factor
        material.diffuse_factor = roughness.roughness_factor;
    }
}

const AtlasAllocation& GLTFFile::parseBaseColorTex(const cgltf_texture* tex,
                                                   AtlasBuilder& tex_builder) {
    const cgltf_image* image = tex->image;
    const cgltf_buffer_view* view = image->buffer_view;

    // Retrieve raw data
    const uint8_t* buffer = ((uint8_t*)view->buffer->data) + view->offset;
    const uint64_t num_bytes = view->size;

    // Parse file, forcing 4 channels (RGBA).
    int width, height;
    int num_channels;

    stbi_uc* pixel_data = stbi_load_from_memory(buffer, num_bytes, &width,
                                                &height, &num_channels, 4);
    assert(pixel_data != nullptr);

    // Load thes 4 channels into our texture
    const AtlasAllocation& alloc = tex_builder.allocateRegion(width, height);

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            const int index = (row * width + col) * 4;
            const TextureColor color =
                TextureColor(pixel_data[index + 0], pixel_data[index + 1],
                             pixel_data[index + 2], pixel_data[index + 3]);

            tex_builder.setColor(col, row, color);
        }
    }

    // Free allocated memory
    stbi_image_free(pixel_data);

    return alloc;
}

} // namespace Graphics
} // namespace Engine