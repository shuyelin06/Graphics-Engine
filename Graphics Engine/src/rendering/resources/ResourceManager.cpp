#include "ResourceManager.h"

#include <fstream>
#include <iostream>

#include <mutex>
#include <regex>
#include <string.h>
#include <unordered_map>
#include <vector>

#include <assert.h>

#include "math/Vector2.h"
#include "math/Vector3.h"

#include "files/FileReader.h"

#include "../Direct3D11.h"
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

class ResourceManagerImpl {
  private:
    struct MeshBuildingJob {
        std::vector<MeshVertex> vertex_data;
        std::vector<MeshTriangle> index_data;
        VertexLayout layout;

        std::shared_ptr<Mesh> mesh = nullptr;
    };
    struct TextureBuildingJob {
        std::vector<uint8_t> data;
        unsigned int width;
        unsigned int height;
        TextureLayout layout;

        bool newTexture = true;
        std::shared_ptr<Texture> texture = nullptr;
    };

  private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    std::vector<std::unique_ptr<MeshPool>> mesh_pools;
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::unordered_map<uint32_t, std::weak_ptr<Mesh>> mesh_map;

    std::vector<std::shared_ptr<Texture>> textures;

    std::vector<MeshBuildingJob> mesh_jobs;
    std::mutex mesh_job_mutex;
    std::vector<TextureBuildingJob> texture_jobs;
    std::mutex texture_job_mutex;

  public:
    ResourceManagerImpl(ID3D11Device* device, ID3D11DeviceContext* context);
    ~ResourceManagerImpl();

    // Initialize System Resources.
    // These are resources that exist for the entire application and are built
    // into the engine.
    void initializeSystemResources();

    // Update Loop.
    // Serve the various requests received by the resource manager.
    void updatePerform();

    // Get Resources
    std::shared_ptr<Mesh> getMesh(int index) const;
    std::shared_ptr<Texture> getTexture(int index) const;

    // Create Resources
    std::shared_ptr<Texture>
    LoadTextureFromFile(const std::string& relative_path);
    std::shared_ptr<Mesh> LoadMeshFromFile(const std::string& relative_path);

    std::shared_ptr<Mesh> requestMesh(const MeshBuilder& mesh_builder);
    std::shared_ptr<Texture>
    requestTexture(const TextureBuilder& tex_builder, bool editable,
                   const std::shared_ptr<Texture>& target);

    // Debug Display
    void imGui();

  private:
    void processMeshJob(const MeshBuildingJob& job);
    void processTextureJob(const TextureBuildingJob& job);

    // System Asset Generation
    void LoadCubeMesh();

    void LoadFallbackColormap();

    bool WriteTextureToPNG(ID3D11Texture2D* texture, std::string path,
                           std::string file);
};

std::unique_ptr<ResourceManager>
ResourceManager::create(ID3D11Device* device, ID3D11DeviceContext* context) {
    std::unique_ptr<ResourceManager> ptr =
        std::unique_ptr<ResourceManager>(new ResourceManager());
    ptr->mImpl = std::make_unique<ResourceManagerImpl>(device, context);
    return ptr;
}
ResourceManager::ResourceManager() = default;
ResourceManager::~ResourceManager() = default;

void ResourceManager::initializeSystemResources() {
    mImpl->initializeSystemResources();
}

void ResourceManager::updatePerform() { mImpl->updatePerform(); }

// Get Resources
std::shared_ptr<Mesh> ResourceManager::getMesh(int index) const {
    return mImpl->getMesh(index);
}
std::shared_ptr<Texture> ResourceManager::getTexture(int index) const {
    return mImpl->getTexture(index);
}

std::shared_ptr<Texture>
ResourceManager::LoadTextureFromFile(const std::string& relative_path) {
    return mImpl->LoadTextureFromFile(relative_path);
}
std::shared_ptr<Mesh>
ResourceManager::LoadMeshFromFile(const std::string& relative_path) {
    return mImpl->LoadMeshFromFile(relative_path);
}

std::shared_ptr<Mesh>
ResourceManager::requestMesh(const MeshBuilder& mesh_builder) {
    return mImpl->requestMesh(mesh_builder);
}
std::shared_ptr<Texture>
ResourceManager::requestTexture(const TextureBuilder& texture_builder,
                                bool editable,
                                const std::shared_ptr<Texture>& target) {
    return mImpl->requestTexture(texture_builder, editable, target);
}

// Debug Display
void ResourceManager::imGui() { mImpl->imGui(); }

ResourceManagerImpl::ResourceManagerImpl(ID3D11Device* device,
                                         ID3D11DeviceContext* context)
    : device(device), context(context) {
    assert(device && context);

    ImGuiHelper::registerImGuiCallback("Render/Resources",
                                       [this]() { imGui(); });
}
ResourceManagerImpl::~ResourceManagerImpl() = default;

// Initialize:
// Loads assets into the asset manager.
void ResourceManagerImpl::initializeSystemResources() {
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

    mesh_pools[MeshPoolType_Terrain]->createGPUResources(device);
    mesh_pools[MeshPoolType_Terrain]->updateGPUResources(context);

    mesh_pools[MeshPoolType_Default]->createGPUResources(device);
    mesh_pools[MeshPoolType_Default]->updateGPUResources(context);
}

void ResourceManagerImpl::updatePerform() {
    {
        std::scoped_lock<std::mutex> mesh_job_lock(mesh_job_mutex);
        while (!mesh_jobs.empty()) {
            processMeshJob(mesh_jobs.back());
            mesh_jobs.pop_back();
        }
    }

    {
        std::scoped_lock<std::mutex> texture_job_lock(texture_job_mutex);
        while (!texture_jobs.empty()) {
            processTextureJob(texture_jobs.back());
            texture_jobs.pop_back();
        }
    }

    // TODO: Might want to throttle this
    for (std::unique_ptr<MeshPool>& pool : mesh_pools) {
        pool->cleanAndCompact();
        pool->updateGPUResources(context);
    }
}

// Get Resources
std::shared_ptr<Mesh> ResourceManagerImpl::getMesh(int index) const {
    assert(0 <= index && index < meshes.size());
    return meshes[index];
}
std::shared_ptr<Texture> ResourceManagerImpl::getTexture(int index) const {
    assert(0 <= index && index < textures.size());
    return textures[index];
}

// LoadTexture:
// Code path for loading all textures.
std::shared_ptr<Texture>
ResourceManagerImpl::LoadTextureFromFile(const std::string& relative_path) {
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
ResourceManagerImpl::LoadMeshFromFile(const std::string& relative_path) {
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

std::shared_ptr<Mesh>
ResourceManagerImpl::requestMesh(const MeshBuilder& mesh_builder) {
    if (mesh_builder.index_buffer.empty() || mesh_builder.vertex_buffer.empty())
        return nullptr;

    std::scoped_lock<std::mutex> mesh_job_lock(mesh_job_mutex);

    MD5Hash md5 = mesh_builder.generateHash();
    // TODO figure out better way to combine the hash values
    const uint32_t hash = md5[0] ^ md5[1] ^ md5[2] ^ md5[3];

    // Attempt to pull mesh from existing map based on hash of the MeshBuilder.
    // This can save a lot of work for duplicate meshes.
    if (auto iter = mesh_map.find(hash); iter != mesh_map.end()) {
        std::weak_ptr<Mesh> meshWeak = mesh_map[hash];
        std::shared_ptr<Mesh> mesh = meshWeak.lock();

        if (mesh) {
            return mesh;
        } else {
            mesh_map.erase(iter);
        }
        // Fallthrough
    }

    MeshBuildingJob& mesh_job = mesh_jobs.emplace_back();

    mesh_job.vertex_data = mesh_builder.vertex_buffer;
    mesh_job.index_data = mesh_builder.index_buffer;
    mesh_job.layout = mesh_builder.layout;

    mesh_job.mesh = std::make_shared<Mesh>();
    mesh_job.mesh->ready = false;

    mesh_map[hash] = mesh_job.mesh;

    return mesh_job.mesh;
}

std::shared_ptr<Texture>
ResourceManagerImpl::requestTexture(const TextureBuilder& tex_builder,
                                    bool editable,
                                    const std::shared_ptr<Texture>& target) {
    std::scoped_lock<std::mutex> lock(texture_job_mutex);

    TextureBuildingJob& job = texture_jobs.emplace_back();
    job.data = tex_builder.data;
    job.width = tex_builder.width;
    job.height = tex_builder.height;
    job.layout = tex_builder.layout;

    // Create a new texture if target == nullptr
    if (target == nullptr) {
        job.newTexture = true;
        job.texture = std::make_shared<Texture>();

        job.texture->width = job.width;
        job.texture->height = job.width;
        job.texture->editable = editable;

        job.texture->ready = false;
    }
    // Update an existing texture if target == nullptr
    else {
        job.newTexture = false;
        job.texture = target;
        job.texture->ready = false;
    }

    return job.texture;
}

// Debug Display
void ResourceManagerImpl::imGui() {
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
            const std::shared_ptr<Mesh> mesh_ptr = mesh;

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
bool ResourceManagerImpl::WriteTextureToPNG(ID3D11Texture2D* texture,
                                            std::string path,
                                            std::string file) {
    PNGFile png_file = PNGFile(path + file);
    return png_file.writePNGData(device, context, texture);
}

void ResourceManagerImpl::processMeshJob(const MeshBuildingJob& job) {
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

        const int poolTriangles =
            max(DEFAULT_POOL_TRIANGLES, job.index_data.size());
        const int poolVertices =
            max(DEFAULT_POOL_VERTICES, job.vertex_data.size());

        mesh_pools.emplace_back(std::make_unique<MeshPool>(
            job.layout, poolTriangles, poolVertices));
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

    // TODO
    // meshes.push_back(mesh);

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

static DXGI_FORMAT TextureLayoutToDXGI(TextureLayout layout) {
    switch (layout) {
    case TextureLayout::R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case TextureLayout::R32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;
    }
}

void ResourceManagerImpl::processTextureJob(const TextureBuildingJob& job) {
    auto& texture = job.texture;

    if (job.newTexture) {
        HRESULT result;

        // Generate my GPU texture resource
        D3D11_TEXTURE2D_DESC tex_desc = {};
        tex_desc.Width = job.width;
        tex_desc.Height = job.height;
        tex_desc.MipLevels = tex_desc.ArraySize = 1;
        tex_desc.Format = TextureLayoutToDXGI(job.layout);
        tex_desc.SampleDesc.Count = 1;
        tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        if (job.texture->editable) {
            tex_desc.Usage = D3D11_USAGE_DYNAMIC;
            tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        } else {
            tex_desc.Usage = D3D11_USAGE_DEFAULT;
            tex_desc.CPUAccessFlags = 0;
        }

        const size_t byteSize = TextureLayoutByteSize(job.layout);
        D3D11_SUBRESOURCE_DATA sr_data = {};
        sr_data.pSysMem = job.data.data();
        sr_data.SysMemPitch = job.width * byteSize; // Bytes per row
        sr_data.SysMemSlicePitch =
            job.width * job.height * byteSize; // Total byte size

        result =
            device->CreateTexture2D(&tex_desc, &sr_data, &texture->texture);
        assert(SUCCEEDED(result));

        // Generate a shader view for my texture
        D3D11_SHADER_RESOURCE_VIEW_DESC tex_view;
        tex_view.Format = TextureLayoutToDXGI(job.layout);
        tex_view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        tex_view.Texture2D.MostDetailedMip = 0;
        tex_view.Texture2D.MipLevels = 1;
        result = device->CreateShaderResourceView(
            job.texture->texture, &tex_view, &(job.texture->shader_view));
        assert(SUCCEEDED(result));
    } else {
        assert(texture->editable);
        assert(job.width == texture->width);
        assert(job.height == texture->height);
        assert(job.layout == texture->layout);

        // Write to my texture using Map / Unmap.
        D3D11_MAPPED_SUBRESOURCE sr;
        context->Map(texture->texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &sr);

        const size_t byteSize = TextureLayoutByteSize(job.layout);
        uint8_t* dest = reinterpret_cast<uint8_t*>(sr.pData);
        const uint8_t* src = reinterpret_cast<const uint8_t*>(job.data.data());

        // We need to copy row-by-row, because while rows are aligned, there may
        // be padding after each row that we're not aware about.
        for (UINT y = 0; y < job.height; ++y) {
            memcpy(dest + y * sr.RowPitch, src + y * job.width * byteSize,
                   job.width * byteSize);
        }

        context->Unmap(texture->texture, 0);
    }

    texture->ready = true;
}

// System Resources
void ResourceManagerImpl::LoadCubeMesh() {
    MeshBuilder builder = MeshBuilder();
    builder.addLayout(POSITION);
    builder.addCube(Vector3(0, 0, 0), Quaternion(), 1.f);

    std::shared_ptr<Mesh> mesh = requestMesh(builder);
    assert(meshes.size() == SystemMesh_Cube);
    meshes.emplace_back(mesh);
}

void ResourceManagerImpl::LoadFallbackColormap() {
    TextureBuilder builder = TextureBuilder(10, 10);
    builder.clear();
    Texture* fallback_tex = builder.generate(device);
    assert(textures.size() == SystemTexture_FallbackColormap);
    textures.push_back(std::shared_ptr<Texture>(fallback_tex));
}

} // namespace Graphics
} // namespace Engine