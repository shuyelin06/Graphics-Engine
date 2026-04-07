#include "ResourceManager.h"

#include <fstream>
#include <iostream>

#include <map>
#include <regex>
#include <string.h>
#include <vector>

#include <assert.h>

#include "math/Vector2.h"
#include "math/Vector3.h"

#include "files/FileReader.h"

#include "../ImGui.h"

// We use the lodepng library to read PNG files.
// See https://github.com/lvandeve/lodepng
#include "lodepng/lodepng.h"

#include "files/GLTFFile.h"
#include "files/PNGFile.h"

using namespace std;

namespace Engine {
using namespace Math;

namespace Graphics {
static const std::string RESOURCE_FOLDER = "data/";

ResourceManager::ResourceManager(ID3D11Device* _device,
                                 ID3D11DeviceContext* _context) {
    device = _device;
    context = _context;
}
ResourceManager::~ResourceManager() = default;

// Initialize:
// Loads assets into the asset manager.
void ResourceManager::initializeSystemResources() {
    // TODO: Be able to create mesh pools on demand
    VertexLayout terrainLayout;
    terrainLayout.addVertexStream(POSITION);
    terrainLayout.addVertexStream(NORMAL);
    mesh_pools.emplace_back(
        std::make_unique<MeshPool>(terrainLayout, 600000, 800000));
    VertexLayout defaultLayout;
    defaultLayout.setAllStreams();
    mesh_pools.emplace_back(
        std::make_unique<MeshPool>(defaultLayout, 100000, 100000));

    // System assets are loaded here
    LoadCubeMesh();
    LoadFallbackColormap();

    mesh_pools[MeshPoolType_Default]->createGPUResources(device);
    mesh_pools[MeshPoolType_Default]->updateGPUResources(context);
}

void ResourceManager::updatePerform(ID3D11DeviceContext* context) {
    std::scoped_lock<std::mutex> job_lock(mesh_job_mutex);

    while (!mesh_jobs.empty()) {
        const auto& job = mesh_jobs.back();
        processMeshJob(job);
        mesh_jobs.pop_back();
    }
}

// Get Resources
std::shared_ptr<Mesh> ResourceManager::getMesh(int index) const {
    assert(0 <= index && index < meshes.size());
    return meshes[index].lock();
}
std::shared_ptr<Texture> ResourceManager::getTexture(int index) const {
    assert(0 <= index && index < textures.size());
    return textures[index];
}

// LoadTexture:
// Code path for loading all textures.
std::shared_ptr<Texture>
ResourceManager::LoadTextureFromFile(const std::string& relative_path) {
    if (relative_path.empty())
        return nullptr;

    const std::string full_path = RESOURCE_FOLDER + relative_path;

    // Matches to find the file name and extension separately.
    // (?:.+/)* matches the path but does not put it in a capture group.
    std::regex name_pattern("(?:.+/)*([a-zA-Z0-9]+)\\.([a-zA-Z]+)");
    smatch match;
    regex_search(relative_path, match, name_pattern);

    Texture* output = nullptr;

    if (match.size() == 3) {
        // If name is ever needed:
        // const std::string name = match[1];
        const std::string extension = match[2];

        FileReader reader = FileReader(full_path);
        if (reader.readFileData()) {
            TextureBuilder builder = TextureBuilder(0, 0);

            if (extension == "png") {
                PNGFile::ReadPNGData(reader.getData(), builder);
                output = builder.generate(device);
            } else
                assert(false); // Unsupported Format
        }
    }

    if (output != nullptr) {
        textures.emplace_back(std::shared_ptr<Texture>(output));
        return textures.back();
    } else {
        return nullptr;
    }
}

std::shared_ptr<Mesh>
ResourceManager::LoadMeshFromFile(const std::string& relative_path) {
    if (relative_path.empty())
        return nullptr;

    const std::string full_path = RESOURCE_FOLDER + relative_path;

    // Matches to find the file name and extension separately.
    // (?:.+/)* matches the path but does not put it in a capture group.
    std::regex name_pattern("(?:.+/)*([a-zA-Z0-9]+)\\.([a-zA-Z]+)");
    smatch match;
    regex_search(relative_path, match, name_pattern);

    std::shared_ptr<Mesh> output = nullptr;

    // If name is ever needed:
    // const std::string name = match[1];
    const std::string extension = match[2];

    if (extension == "glb" || extension == "gltf") {
        MeshBuilder builder = MeshBuilder();
        GLTFFile::ReadGLTFMesh(full_path, builder);
        output = requestMesh(builder);
    } else
        assert(false); // Unsupported Format

    if (output != nullptr) {
        meshes.emplace_back(output);
        return output;
    } else
        return nullptr;
}

std::shared_ptr<TextureBuilder> ResourceManager::createTextureBuilder() {
    return std::make_shared<TextureBuilder>(1, 1);
}

MeshPool* ResourceManager::getMeshPool(MeshPoolType pool_type) {
    return mesh_pools[pool_type].get();
}

std::shared_ptr<Mesh>
ResourceManager::requestMesh(const MeshBuilder& mesh_builder) {
    if (mesh_builder.index_buffer.empty() || mesh_builder.vertex_buffer.empty())
        return nullptr;

    std::scoped_lock<std::mutex> mesh_job_lock(mesh_job_mutex);

    MeshBuildingJob& mesh_job = mesh_jobs.emplace_back();

    mesh_job.vertex_data = mesh_builder.vertex_buffer;
    mesh_job.index_data = mesh_builder.index_buffer;
    mesh_job.layout = mesh_builder.layout;

    mesh_job.mesh = std::make_shared<Mesh>();
    mesh_job.mesh->ready = false;

    return mesh_job.mesh;
}

std::shared_ptr<VSTerrain> ResourceManager::requestTerrainMesh() {
    std::shared_ptr<VSTerrain> mesh = std::make_shared<VSTerrain>();
    mesh->initialize(device, context);
    return mesh;
}

// Debug Display
void ResourceManager::imGui() {
#if defined(IMGUI_ENABLED)
    for (const auto& mesh_pool : mesh_pools) {
        ImGui::SeparatorText("Mesh Pool");
        ImGui::Indent();
        {
            ImGui::Text("Allocations: %zu", mesh_pool->meshes.size());
            ImGui::Text("Vertex Count: %u", mesh_pool->vertex_size);
            ImGui::Text("Triangle Count: %u", mesh_pool->triangle_size);
        }
        ImGui::Unindent();
    }

    ImGui::Text("Mesh Count: %zu", meshes.size());
    if (ImGui::BeginTable("Mesh Information", 3)) {
        ImGui::TableSetupColumn("Index");
        ImGui::TableSetupColumn("Vertex Count");
        ImGui::TableSetupColumn("Index Count");
        ImGui::TableHeadersRow();

        int mesh_index = 0;
        for (const auto& mesh : meshes) {
            const std::shared_ptr<Mesh> mesh_ptr = mesh.lock();

            if (!mesh_ptr)
                continue;

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%i", mesh_index++);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", mesh_ptr->num_triangles * 3);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%zu", mesh_ptr->num_vertices);
        }

        ImGui::EndTable();
    }

    ImGui::Text("Texture Count: %zu", textures.size());
#endif
}

// WriteTextureToPNG:
// Uses the PNGFile interface to write a texture to a PNG file
bool ResourceManager::WriteTextureToPNG(ID3D11Texture2D* texture,
                                        std::string path, std::string file) {
    PNGFile png_file = PNGFile(path + file);
    return png_file.writePNGData(device, context, texture);
}

void ResourceManager::processMeshJob(const MeshBuildingJob& job) {
    // Iterate through my available mesh pools. Check for:
    // 1) Pools with the same layout
    // 2) Pools with space
    // If we do not find a pool, we create a new one.
    MeshPool* pool = nullptr;
    for (const auto& mesh_pool : mesh_pools) {
        const bool layout_match = (mesh_pool->layout == job.layout);
        const bool has_vertex_space =
            job.vertex_data.size() + mesh_pool->vertex_size <=
            mesh_pool->vertex_capacity;
        const bool has_index_space =
            job.index_data.size() + mesh_pool->triangle_size <=
            mesh_pool->triangle_capacity;

        if (layout_match && has_vertex_space && has_index_space) {
            pool = mesh_pool.get();
        }
    }

    if (!pool) {
        constexpr int DEFAULT_POOL_TRIANGLES = 100000;
        constexpr int DEFAULT_POOL_VERTICES = 100000;

        mesh_pools.emplace_back(std::make_unique<MeshPool>(
            job.layout, DEFAULT_POOL_TRIANGLES, DEFAULT_POOL_VERTICES));
        pool = mesh_pools.back().get();
    }

    assert(pool);

    if (!pool->has_gpu_resources) {
        pool->createGPUResources(device);
    }

    assert(pool->has_gpu_resources); // Must call createGPUResources

    // Copy to CPU-side index and vertex buffers
    memcpy(pool->cpu_ibuffer.get() + pool->triangle_size * sizeof(MeshTriangle),
           job.index_data.data(), job.index_data.size() * sizeof(MeshTriangle));

    // Upload my vertex buffer data. We have to allocate based on pool's
    // layout to keep the vertices aligned. This means that space could be
    // wasted if the pool supports streams that the builder does not have.
    // This array should match the vertex streams.
    for (int i = 0; i < BINDABLE_STREAM_COUNT; i++) {
        if (pool->layout.hasVertexStream((VertexDataStream)i)) {
            const UINT byte_size =
                VertexLayout::VertexStreamStride((VertexDataStream)i);

            // Now, for each vertex, I will pull the data I want for my
            // stream and then copy it to the end of my buffer.
            for (int j = 0; j < job.vertex_data.size(); j++) {
                const void* address =
                    job.vertex_data[j].GetAddressOf((VertexDataStream)i);

                // Also copy to my CPU-side copy of the data
                memcpy(pool->cpu_vbuffers[i].get() +
                           (pool->vertex_size + j) * byte_size,
                       address, byte_size);
            }
        }
    }

    // Create my mesh
    const std::shared_ptr<Mesh>& mesh = job.mesh;
    pool->meshes.emplace_back(mesh);
    mesh->buffer_pool = pool;
    mesh->layout = job.layout;
    mesh->vertex_start = pool->vertex_size;
    mesh->num_vertices = job.vertex_data.size();
    mesh->triangle_start = pool->triangle_size;
    mesh->num_triangles = job.index_data.size();

    for (const MeshVertex& vertex : job.vertex_data)
        mesh->aabb.expandToContain(vertex.position);

    // Update my mesh pool
    pool->vertex_size += job.vertex_data.size();
    pool->triangle_size += job.index_data.size();

    // Upload to GPU
    pool->updateGPUResources(context);

    // Done. Pop the job and mark the mesh as ready.
    mesh->ready = true;
}

// System Resources
void ResourceManager::LoadCubeMesh() {
    MeshBuilder builder = MeshBuilder();
    builder.addLayout(POSITION);
    builder.addCube(Vector3(0, 0, 0), Quaternion(), 1.f);

    std::shared_ptr<Mesh> mesh = requestMesh(builder);
    assert(meshes.size() == SystemMesh_Cube);
    meshes.emplace_back(mesh);
}

void ResourceManager::LoadFallbackColormap() {
    TextureBuilder builder = TextureBuilder(10, 10);
    builder.clear({90, 34, 139, 255});
    Texture* fallback_tex = builder.generate(device);
    assert(textures.size() == SystemTexture_FallbackColormap);
    textures.push_back(std::shared_ptr<Texture>(fallback_tex));
}

} // namespace Graphics
} // namespace Engine