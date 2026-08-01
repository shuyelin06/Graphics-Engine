#include "ResourceManager.h"

#include <fstream>
#include <iostream>

#include <deque>
#include <mutex>
#include <regex>
#include <string.h>
#include <unordered_map>
#include <vector>

#include <assert.h>

#include "core/JobGraph.h"
#include "core/UniqueFunction.h"

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

namespace Engine
{
using namespace Math;

namespace Graphics
{
static const std::string RESOURCE_FOLDER = "data/";

static const char* kIOPathToken = "IOFilePath";
static const char* kTextureTargetToken = "TextureTarget";
static const char* kTextureDataToken = "TextureData";

struct IOPathParam
{
    std::string path;
    IOPathParam(const std::string& path)
        : path(path)
    {
    }
};
struct TextureTargetParam
{
    std::shared_ptr<Texture> texture;
    TextureTargetParam(std::shared_ptr<Texture>& texture)
        : texture(texture)
    {
    }
};
struct TextureDataParam
{
    std::vector<uint8_t> data;
    TextureDataParam() = default;
    TextureDataParam(const std::vector<uint8_t>& data)
        : data(data)
    {
    }
};

enum class BuildJobStatus : uint8_t
{
    Failed = -2,
    Invalid = -1,
    PendingResourceData = 0,
    PendingGPUCreation = 1,
    PendingGPUUpload = 2,
    Ready = 3,
};
struct MeshBuildingJob
{
    std::vector<MeshVertex> vertex_data;
    std::vector<MeshTriangle> index_data;
    VertexLayout layout;

    std::shared_ptr<Mesh> mesh = nullptr;
};
struct TextureBuildingJob
{
    BuildJobStatus status = BuildJobStatus::Invalid;

    // Status: PendingResourceData.
    // Reads data from this path into the resource folder.
    std::string resourceFilePath{};

    // Status: PendingGPUUpload
    // Uploads this data to the GPU and marks texture as ready
    std::vector<uint8_t> data{};

    std::shared_ptr<Texture> texture = nullptr;

    TextureBuildingJob() = default;
};

struct DebugState
{
    struct SelectedTexture
    {
        std::weak_ptr<Texture> textureWeak;
        bool close = false;
    };
    std::vector<SelectedTexture> selectedTextures;
};

class ResourceManagerImpl
{
  private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    std::unique_ptr<DebugState> debugState = nullptr;

    // Resources owned by ResourceManager
    std::vector<std::unique_ptr<MeshPool>> mesh_pools;
    std::shared_ptr<Texture> fallbackColormap;
    std::shared_ptr<Mesh> cubeMesh;

    // Weak tracking of resources for reuse and debugging
    std::vector<std::weak_ptr<Texture>> textures;
    std::unordered_map<uint32_t, std::weak_ptr<Mesh>> mesh_map;

    // Job Management
    // Tracks the state of resource generation
    std::vector<std::unique_ptr<JobGraph>> jobs;

    std::vector<MeshBuildingJob> mesh_jobs;
    std::mutex mesh_job_mutex;

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
    std::shared_ptr<Texture> getFallbackColormap() const;

    // Create Resources
    std::shared_ptr<Mesh> LoadMeshFromFile(const std::string& relative_path);

    std::shared_ptr<Mesh> requestMesh(const MeshBuilder& mesh_builder);
    std::shared_ptr<Texture> requestTexture(const TextureRequestParams& params);

    void clearDepthStencil(const Texture& texture);

    // Debug Display
    void imGui();

  private:
    void processMeshJob(const MeshBuildingJob& job);

    // In: kIOPathToken, kTextureTargetToken. Out: kTextureDataToken, kTextureTargetToken
    bool textureJobReadIOData(JobGraphMemoryPool& pool);
    // In: kTextureTargetToken. Out: kTextureTargetToken
    bool textureJobPendingGPUCreation(JobGraphMemoryPool& pool);
    // In: kTextureTargetToken. Out: None
    bool textureJobPendingGPUUpload(JobGraphMemoryPool& pool);

    // System Asset Generation
    void LoadCubeMesh();

    void LoadFallbackColormap();

    bool WriteTextureToPNG(ID3D11Texture2D* texture,
                           std::string path,
                           std::string file);
};

std::unique_ptr<ResourceManager>
ResourceManager::create(ID3D11Device* device, ID3D11DeviceContext* context)
{
    std::unique_ptr<ResourceManager> ptr =
        std::unique_ptr<ResourceManager>(new ResourceManager());
    ptr->mImpl = std::make_unique<ResourceManagerImpl>(device, context);
    return ptr;
}
ResourceManager::ResourceManager() = default;
ResourceManager::~ResourceManager() = default;

void ResourceManager::initializeSystemResources()
{
    mImpl->initializeSystemResources();
}

void ResourceManager::updatePerform()
{
    mImpl->updatePerform();
}

std::shared_ptr<Texture> ResourceManager::getFallbackColormap() const
{
    return mImpl->getFallbackColormap();
}

std::shared_ptr<Mesh>
ResourceManager::LoadMeshFromFile(const std::string& relative_path)
{
    return mImpl->LoadMeshFromFile(relative_path);
}

std::shared_ptr<Mesh>
ResourceManager::requestMesh(const MeshBuilder& mesh_builder)
{
    return mImpl->requestMesh(mesh_builder);
}
std::shared_ptr<Texture>
ResourceManager::requestTexture(const TextureRequestParams& params)
{
    return mImpl->requestTexture(params);
}

void ResourceManager::clearDepthStencil(const Texture& texture)
{
    mImpl->clearDepthStencil(texture);
}

// Debug Display
void ResourceManager::imGui()
{
    mImpl->imGui();
}

ResourceManagerImpl::ResourceManagerImpl(ID3D11Device* device,
                                         ID3D11DeviceContext* context)
    : device(device)
    , context(context)
{
    assert(device && context);

    ImGuiHelper::registerImGuiCallback("Render/Resources",
                                       [this]() { imGui(); });

    debugState = std::make_unique<DebugState>();
}
ResourceManagerImpl::~ResourceManagerImpl() = default;

// Initialize:
// Loads assets into the asset manager.
void ResourceManagerImpl::initializeSystemResources()
{
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

void ResourceManagerImpl::updatePerform()
{
    {
        std::scoped_lock<std::mutex> mesh_job_lock(mesh_job_mutex);
        while (!mesh_jobs.empty())
        {
            processMeshJob(mesh_jobs.back());
            mesh_jobs.pop_back();
        }
    }

    {
        auto iter = jobs.begin();
        while (iter != jobs.end())
        {
            JobGraph* job = (*iter).get();
            job->processSynchronousJobs();

            if (job->isDone())
            {
                iter = jobs.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    // TODO: Might want to throttle this
    for (std::unique_ptr<MeshPool>& pool : mesh_pools)
    {
        pool->cleanAndCompact();
        pool->updateGPUResources(context);
    }
}

// Get Resources
std::shared_ptr<Texture> ResourceManagerImpl::getFallbackColormap() const
{
    return fallbackColormap;
}

std::shared_ptr<Mesh>
ResourceManagerImpl::LoadMeshFromFile(const std::string& relative_path)
{
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

    if (extension == "glb" || extension == "gltf")
    {
        MeshBuilder builder = MeshBuilder();
        GLTFFile::ReadGLTFMesh(full_path, builder);
        output = requestMesh(builder);
    }
    else
        assert(false); // Unsupported Format

    return output;
}

std::shared_ptr<Mesh>
ResourceManagerImpl::requestMesh(const MeshBuilder& mesh_builder)
{
    if (mesh_builder.index_buffer.empty() || mesh_builder.vertex_buffer.empty())
        return nullptr;

    std::scoped_lock<std::mutex> mesh_job_lock(mesh_job_mutex);

    MD5Hash md5 = mesh_builder.generateHash();
    // TODO figure out better way to combine the hash values
    const uint32_t hash = md5[0] ^ md5[1] ^ md5[2] ^ md5[3];

    // Attempt to pull mesh from existing map based on hash of the MeshBuilder.
    // This can save a lot of work for duplicate meshes.
    if (auto iter = mesh_map.find(hash); iter != mesh_map.end())
    {
        std::weak_ptr<Mesh> meshWeak = mesh_map[hash];
        std::shared_ptr<Mesh> mesh = meshWeak.lock();

        if (mesh)
        {
            return mesh;
        }
        else
        {
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
ResourceManagerImpl::requestTexture(const TextureRequestParams& params)
{
    // TODO Thread Safe Please
    jobs.emplace_back(std::make_unique<JobGraph>());
    JobGraph* graph = jobs.back().get();

    // Determine my target.
    std::shared_ptr<Texture> texture = nullptr;
    JobGraph::JobID gpuCreationJob = JobGraph::kInvalidJobID;

    if (auto targetExisting = std::get_if<TextureRequestParams::TargetExisting>(
            &params.targetSettings))
    {
        texture = targetExisting->target;
    }
    else if (auto targetNew = std::get_if<TextureRequestParams::TargetNew>(
                 &params.targetSettings))
    {
        texture = std::make_shared<Texture>();
        // Track in textures vector
        textures.push_back(texture);

        texture->debugName = targetNew->debugName;
        texture->layout = targetNew->layout;
        texture->mips = targetNew->mipLevels;
        texture->editable = targetNew->editable;

        // Because we have a new texture, we must create a job to create on the GPU.
        gpuCreationJob = graph->createJob(
            JobGraphContext::kSynchronous, [this](JobGraphMemoryPool& pool) {
                return textureJobPendingGPUCreation(pool);
            });
    }
    else
    {
        throw std::runtime_error("Invalid target setting!");
    }

    // Determine my data source.
    JobGraph::JobID dataJob = JobGraph::kInvalidJobID;

    if (auto dataFromFile = std::get_if<TextureRequestParams::DataFromFile>(
            &params.dataSettings))
    {
        std::unique_ptr<IOPathParam> pathParam =
            std::make_unique<IOPathParam>(RESOURCE_FOLDER + dataFromFile->path);
        graph->storeMemory(kIOPathToken, std::move(pathParam));

        dataJob = graph->createJob(JobGraphContext::kAsync,
                                   [this](JobGraphMemoryPool& pool) {
                                       return textureJobReadIOData(pool);
                                   });
    }
    else if (auto dataFromBuilder =
                 std::get_if<TextureRequestParams::DataFromBuilder>(
                     &params.dataSettings))
    {
        const auto& builder = *dataFromBuilder->builder;

        std::unique_ptr<TextureDataParam> dataParam =
            std::make_unique<TextureDataParam>(builder.getData());
        texture->layout = builder.getLayout();
        texture->width = builder.getWidth();
        texture->height = builder.getHeight();
        texture->mips = builder.getNumMips();

        graph->storeMemory(kTextureDataToken, std::move(dataParam));
    }
    else
    {
        throw std::runtime_error("Invalid data setting!");
    }

    std::unique_ptr<TextureTargetParam> targetParam =
        std::make_unique<TextureTargetParam>(texture);
    graph->storeMemory(kTextureTargetToken, std::move(targetParam));

    JobGraph::JobID gpuUploadJob = graph->createJob(
        JobGraphContext::kSynchronous, [this](JobGraphMemoryPool& pool) {
            return textureJobPendingGPUUpload(pool);
        });

    if (dataJob != JobGraph::kInvalidJobID)
    {
        if (gpuCreationJob != JobGraph::kInvalidJobID)
        {
            graph->registerDependency(dataJob, gpuCreationJob);
            graph->registerDependency(gpuCreationJob, gpuUploadJob);
        }
        else
        {
            graph->registerDependency(dataJob, gpuUploadJob);
        }
    }
    else
    {
        if (gpuCreationJob != JobGraph::kInvalidJobID)
        {
            graph->registerDependency(gpuCreationJob, gpuUploadJob);
        }
        else
        {
            // Nothing
        }
    }

    graph->kickoff();

    return texture;
}

void ResourceManagerImpl::clearDepthStencil(const Texture& texture)
{
    assert(texture.depth_view != nullptr);
    context->ClearDepthStencilView(texture.depth_view, D3D11_CLEAR_DEPTH, 1.0f,
                                   0);
}

// Debug Display
void ResourceManagerImpl::imGui()
{
#if defined(IMGUI_ENABLED)
    if (ImGui::CollapsingHeader("Mesh View"))
    {
        for (const auto& mesh_pool : mesh_pools)
        {
            ImGui::SeparatorText("Mesh Pool");
            ImGui::Indent();
            {
                ImGui::Text("Allocations: %zu", mesh_pool->meshes.size());
                ImGui::Text("Vertex Count: %u", mesh_pool->vertex_size);
                ImGui::Text("Triangle Count: %u", mesh_pool->triangle_size);
            }
            ImGui::Unindent();
        }

        ImGui::Text("Mesh Count: %zu", mesh_map.size());
        if (ImGui::BeginTable("Mesh Information", 3))
        {
            ImGui::TableSetupColumn("Index");
            ImGui::TableSetupColumn("Vertex Count");
            ImGui::TableSetupColumn("Index Count");
            ImGui::TableHeadersRow();

            int mesh_index = 0;
            for (const auto& pair : mesh_map)
            {
                const std::shared_ptr<Mesh> mesh_ptr = pair.second.lock();

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
    }

    if (ImGui::CollapsingHeader("Texture View"))
    {
        ImGui::Text("Texture Count: %zu", textures.size());
        ImGui::Indent();
        ImGui::Text("Note: Hover over a row to see the texture");
        ImGui::Unindent();

        if (ImGui::BeginTable("Texture Information", 5))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Width");
            ImGui::TableSetupColumn("Height");
            ImGui::TableSetupColumn("Mips");
            ImGui::TableSetupColumn("Editable");
            ImGui::TableHeadersRow();

            auto iter = textures.begin();
            while (iter != textures.end())
            {
                const std::shared_ptr<Texture> texture = (*iter).lock();
                bool selected = false;

                if (texture != nullptr)
                {

                    ImGui::TableNextRow();

                    // Note: We configure row as selectable
                    ImGui::TableSetColumnIndex(0);
                    const std::string name =
                        texture->debugName.empty() ? "?" : texture->debugName;
                    if (ImGui::Selectable(
                            name.c_str(), false,
                            ImGuiSelectableFlags_SpanAllColumns |
                                ImGuiSelectableFlags_AllowOverlap))
                    {
                        debugState->selectedTextures.push_back(
                            {texture, false});
                    }

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%zu", texture->width);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%zu", texture->height);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%zu", texture->mips);
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text(texture->editable ? "T" : "F");

                    ++iter;
                }
                else
                {
                    iter = textures.erase(iter);
                }
            }

            ImGui::EndTable();
        }
    }

    auto iter = debugState->selectedTextures.begin();
    while (iter != debugState->selectedTextures.end())
    {
        auto selectedTexture = *iter;
        std::shared_ptr<Texture> texture = selectedTexture.textureWeak.lock();

        if (ImGui::Begin(("Texture" + texture->debugName).c_str(),
                         &selectedTexture.close))
        {
            texture->displayImGui(256);
            ImGui::End();
        }

        if (texture == nullptr || selectedTexture.close)
        {
            iter = debugState->selectedTextures.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
#endif
}

// WriteTextureToPNG:
// Uses the PNGFile interface to write a texture to a PNG file
bool ResourceManagerImpl::WriteTextureToPNG(ID3D11Texture2D* texture,
                                            std::string path,
                                            std::string file)
{
    PNGFile png_file = PNGFile(path + file);
    return png_file.writePNGData(device, context, texture);
}

void ResourceManagerImpl::processMeshJob(const MeshBuildingJob& job)
{
    // Iterate through my available mesh pools. Check for:
    // 1) Pools with the same layout
    // 2) Pools with space
    // If we do not find a pool, we create a new one.
    MeshPool* pool = nullptr;
    for (const auto& mesh_pool : mesh_pools)
    {
        const bool layout_match = (mesh_pool->layout == job.layout);
        const bool has_vertex_space =
            job.vertex_data.size() + mesh_pool->vertex_size <=
            mesh_pool->vertex_capacity;
        const bool has_index_space =
            job.index_data.size() + mesh_pool->triangle_size <=
            mesh_pool->triangle_capacity;

        if (layout_match && has_vertex_space && has_index_space)
        {
            pool = mesh_pool.get();
        }
    }

    if (!pool)
    {
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

    if (!pool->has_gpu_resources)
    {
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
    for (int i = 0; i < BINDABLE_STREAM_COUNT; i++)
    {
        if (pool->layout.hasVertexStream((VertexDataStream)i))
        {
            const UINT byte_size =
                VertexLayout::VertexStreamStride((VertexDataStream)i);

            // Now, for each vertex, I will pull the data I want for my
            // stream and then copy it to the end of my buffer.
            for (int j = 0; j < job.vertex_data.size(); j++)
            {
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

static DXGI_FORMAT TextureLayoutToDXGI(TextureLayout layout)
{
    switch (layout)
    {
    case TextureLayout::R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case TextureLayout::R32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;
    }
    return DXGI_FORMAT_UNKNOWN;
}

bool ResourceManagerImpl::textureJobReadIOData(JobGraphMemoryPool& pool)
{
    std::unique_ptr<IOPathParam> pathParam =
        pool.load<IOPathParam>(kIOPathToken);
    std::unique_ptr<TextureTargetParam> targetParam =
        pool.load<TextureTargetParam>(kTextureTargetToken);

    const std::string& path = pathParam->path;
    auto& texture = targetParam->texture;

    // RESOURCE_FOLDER + job.resourceFilePath;

    // Matches to find the file name and extension separately.
    // (?:.+/)* matches the path but does not put it in a capture group.
    std::regex name_pattern("(?:.+/)*([a-zA-Z0-9]+)\\.([a-zA-Z]+)");
    smatch match;
    regex_search(path, match, name_pattern);

    std::unique_ptr<TextureDataParam> dataParam = nullptr;

    bool success = false;
    if (match.size() == 3)
    {
        // If name is ever needed:
        // const std::string name = match[1];
        const std::string extension = match[2];

        FileReader reader = FileReader(path);
        if (reader.readFileData())
        {
            if (extension == "png")
            {
                TextureBuilder builder = PNGFile::ReadPNGData(reader.getData());

                texture->layout = builder.getLayout();
                texture->width = builder.getWidth();
                texture->height = builder.getHeight();
                texture->mips = builder.getNumMips();

                dataParam =
                    std::make_unique<TextureDataParam>(builder.getData());

                success = true;
            }
        }
    }

    if (success)
    {
        pool.store(kTextureTargetToken, std::move(targetParam));
        pool.store(kTextureDataToken, std::move(dataParam));
    }

    return success;
}

bool ResourceManagerImpl::textureJobPendingGPUCreation(JobGraphMemoryPool& pool)
{
    std::unique_ptr<TextureTargetParam> targetParam =
        pool.load<TextureTargetParam>(kTextureTargetToken);

    auto& texture = targetParam->texture;

    // Creation settings
    HRESULT result;

    // Generate my GPU texture resource
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.Width = texture->width;
    tex_desc.Height = texture->height;
    tex_desc.MipLevels = texture->mips;
    tex_desc.ArraySize = 1;
    tex_desc.Format = TextureLayoutToDXGI(texture->layout);
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage =
        texture->editable ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    tex_desc.CPUAccessFlags = texture->editable ? D3D11_CPU_ACCESS_WRITE : 0;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    // Note: We do not pass initial data here. We will set the data in the
    // upload stage for easier reasoning about (yes, more inefficient but that
    // is not the primary focus right now)
    result = device->CreateTexture2D(&tex_desc, nullptr, &texture->texture);
    if (!SUCCEEDED(result))
        return false;

    // Generate a shader view for my texture
    D3D11_SHADER_RESOURCE_VIEW_DESC tex_view = {};
    tex_view.Format = TextureLayoutToDXGI(texture->layout);
    tex_view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    tex_view.Texture2D.MostDetailedMip = 0;
    tex_view.Texture2D.MipLevels = texture->mips;
    result = device->CreateShaderResourceView(texture->texture, &tex_view,
                                              &(texture->shader_view));
    if (!SUCCEEDED(result))
        return false;

    pool.store(kTextureTargetToken, std::move(targetParam));

    return true;
}

bool ResourceManagerImpl::textureJobPendingGPUUpload(JobGraphMemoryPool& pool)
{
    std::unique_ptr<TextureTargetParam> targetParam =
        pool.load<TextureTargetParam>(kTextureTargetToken);
    std::unique_ptr<TextureDataParam> dataParam =
        pool.load<TextureDataParam>(kTextureDataToken);

    auto& texture = targetParam->texture;
    auto& data = dataParam->data;

    // Creation settings
    const bool isEditableTexture = texture->editable;
    const unsigned int numMips = texture->mips;

    const size_t byteSize = TextureLayoutByteSize(texture->layout);

    // 2 Options:
    // - Editable Texture. We can map / unmap.
    // - Non-Editable Texture. We have to UpdateSubresource (TODO)
    // Ideally textures that are edited a lot should be marked as editable so we
    // can do mapping / unmapping which will have less stalling.
    if (isEditableTexture)
    {
        assert(numMips == 1); // TODO: Unimplemented for multiple mips

        // Write to my texture using Map / Unmap.
        D3D11_MAPPED_SUBRESOURCE sr;
        context->Map(texture->texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &sr);

        uint8_t* dest = reinterpret_cast<uint8_t*>(sr.pData);
        const uint8_t* src = reinterpret_cast<const uint8_t*>(data.data());

        // We need to copy row-by-row, because while rows are aligned, there
        // may be padding after each row that we're not aware about.
        for (UINT y = 0; y < texture->height; ++y)
        {
            memcpy(dest + y * sr.RowPitch, src + y * texture->width * byteSize,
                   texture->width * byteSize);
        }

        context->Unmap(texture->texture, 0);
    }
    else
    {
        uint8_t* dataSrc = data.data();
        unsigned int mipWidth = texture->width;
        unsigned int mipHeight = texture->height;

        for (int i = 0; i < numMips; i++)
        {
            const unsigned int mipLevel = i;
            context->UpdateSubresource(texture->texture, mipLevel, nullptr,
                                       dataSrc, mipWidth * byteSize, 0);

            dataSrc += mipWidth * mipHeight * byteSize;
            mipWidth = max(1, mipWidth / 2);
            mipHeight = max(1, mipHeight / 2);
        }
    }

    texture->ready = true;

    return true;
}

// System Resources
void ResourceManagerImpl::LoadCubeMesh()
{
    MeshBuilder builder = MeshBuilder();
    builder.addLayout(POSITION);
    builder.addCube(Vector3(0, 0, 0), Quaternion(), 1.f);
    cubeMesh = requestMesh(builder);
}

void ResourceManagerImpl::LoadFallbackColormap()
{
    TextureBuilder builder =
        TextureBuilder(1, 1, TextureLayout::R8G8B8A8_UNORM);
    builder.clear();
    TextureRequestParams request;
    auto& targetSettings = request.targetUseNew();
    targetSettings.debugName = "Fallback Colormap";
    targetSettings.editable = false;
    targetSettings.layout = TextureLayout::R8G8B8A8_UNORM;
    auto& dataSettings = request.dataFromBuilder();
    dataSettings.builder = &builder;
    fallbackColormap = requestTexture(request);
}

} // namespace Graphics
} // namespace Engine