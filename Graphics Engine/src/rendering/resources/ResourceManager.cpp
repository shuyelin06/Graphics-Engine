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

    std::shared_ptr<Geometry> mesh = nullptr;
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
    Device* device;
    DeviceContext* context;

    std::unique_ptr<DebugState> debugState = nullptr;

    // Resources owned by ResourceManager
    std::vector<std::unique_ptr<MeshPool>> mesh_pools;
    std::shared_ptr<Texture> fallbackColormap;
    std::shared_ptr<Geometry> cubeMesh;

    // Weak tracking of resources for reuse and debugging
    std::vector<std::weak_ptr<Texture>> textures;
    std::unordered_map<uint32_t, std::weak_ptr<Geometry>> mesh_map;

    // Job Management
    // Tracks the state of resource generation
    std::vector<MeshBuildingJob> mesh_jobs;
    std::mutex mesh_job_mutex;

  public:
    ResourceManagerImpl(Device* device, DeviceContext* context);
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
    std::shared_ptr<Geometry> getCubeMesh() const;

    // Create Resources
    std::shared_ptr<Geometry>
    LoadMeshFromFile(const std::string& relative_path);

    std::shared_ptr<Geometry> requestMesh(const MeshBuilder& mesh_builder);
    std::shared_ptr<Texture> requestTexture(const char* path);

    // Debug Display
    void imGui();

  private:
    void processMeshJob(const MeshBuildingJob& job);

    // System Asset Generation
    void LoadCubeMesh();

    void LoadFallbackColormap();

    bool WriteTextureToPNG(ID3D11Texture2D* texture,
                           std::string path,
                           std::string file);
};

std::unique_ptr<ResourceManager> ResourceManager::create(Device* device,
                                                         DeviceContext* context)
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

void ResourceManager::updatePerform() { mImpl->updatePerform(); }

std::shared_ptr<Texture> ResourceManager::getFallbackColormap() const
{
    return mImpl->getFallbackColormap();
}
std::shared_ptr<Geometry> ResourceManager::getCubeMesh() const
{
    return mImpl->getCubeMesh();
}

std::shared_ptr<Geometry>
ResourceManager::LoadMeshFromFile(const std::string& relative_path)
{
    return mImpl->LoadMeshFromFile(relative_path);
}

std::shared_ptr<Geometry>
ResourceManager::requestMesh(const MeshBuilder& mesh_builder)
{
    return mImpl->requestMesh(mesh_builder);
}
std::shared_ptr<Texture> ResourceManager::requestTexture(const char* path)
{
    return mImpl->requestTexture(path);
}

// Debug Display
void ResourceManager::imGui() { mImpl->imGui(); }

ResourceManagerImpl::ResourceManagerImpl(Device* device, DeviceContext* context)
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
    // System assets are loaded here
    LoadCubeMesh();
    LoadFallbackColormap();
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

    // TODO: Might want to throttle this
    for (std::unique_ptr<MeshPool>& pool : mesh_pools)
    {
        pool->cleanAndCompact();
        pool->updateGPUResources(context);

        // TEmp
        for (auto meshWeak : pool->meshes)
        {
            std::shared_ptr<Geometry> meshStrong = meshWeak.lock();
            if (meshStrong)
            {
                meshStrong->ready = true;
            }
        }
    }
}

// Get Resources
std::shared_ptr<Texture> ResourceManagerImpl::getFallbackColormap() const
{
    return fallbackColormap;
}
std::shared_ptr<Geometry> ResourceManagerImpl::getCubeMesh() const
{
    return cubeMesh;
}

std::shared_ptr<Geometry>
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

    std::shared_ptr<Geometry> output = nullptr;

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

std::shared_ptr<Geometry>
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
        std::weak_ptr<Geometry> meshWeak = mesh_map[hash];
        std::shared_ptr<Geometry> mesh = meshWeak.lock();

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

    mesh_job.mesh = std::make_shared<Geometry>();
    mesh_job.mesh->ready = false;

    mesh_map[hash] = mesh_job.mesh;

    return mesh_job.mesh;
}

std::shared_ptr<Texture> ResourceManagerImpl::requestTexture(const char* path)
{
    const std::string pathStr = RESOURCE_FOLDER + path;

    // Matches to find the file name and extension separately.
    // (?:.+/)* matches the path but does not put it in a capture group.
    std::regex name_pattern("(?:.+/)*([a-zA-Z0-9]+)\\.([a-zA-Z]+)");
    smatch match;
    regex_search(pathStr, match, name_pattern);

    std::shared_ptr<Texture> texture = nullptr;

    bool success = false;
    if (match.size() == 3)
    {
        // If name is ever needed:
        // const std::string name = match[1];
        const std::string extension = match[2];

        FileReader reader = FileReader(pathStr);
        if (reader.readFileData())
        {
            if (extension == "png")
            {
                TextureBuilder builder = PNGFile::ReadPNGData(reader.getData());
                texture = device->createTexture(
                    "", builder.getLayout(),
                    TextureUsage::ShaderResource | TextureUsage::RenderTarget,
                    builder.getWidth(), builder.getHeight(), 1, 1, false,
                    builder.getData().data());
                context->generateMips(texture);
                success = true;
            }
        }
    }

    return texture;
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
                ImGui::Text("Index Count: %u", mesh_pool->index_size);
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
                const std::shared_ptr<Geometry> mesh_ptr = pair.second.lock();

                if (!mesh_ptr)
                    continue;

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%i", mesh_index++);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", mesh_ptr->vertexCount);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%zu", mesh_ptr->indexCount);
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
                    const std::string name = "?";
                    if (ImGui::Selectable(
                            name.c_str(), false,
                            ImGuiSelectableFlags_SpanAllColumns |
                                ImGuiSelectableFlags_AllowOverlap))
                    {
                        debugState->selectedTextures.push_back(
                            {texture, false});
                    }

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%zu", texture->getWidth());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%zu", texture->getHeight());
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%zu", texture->getMips());
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("?");

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

        if (ImGui::Begin("Texture --", &selectedTexture.close))
        {
            texture->doImgui();
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
    return png_file.writePNGData(device->getDevice(), context->getContext(),
                                 texture);
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
            job.index_data.size() * 3 + mesh_pool->index_size <=
            mesh_pool->index_capacity;

        if (layout_match && has_vertex_space && has_index_space)
        {
            pool = mesh_pool.get();
        }
    }

    if (!pool)
    {
        constexpr int DEFAULT_POOL_TRIANGLES = 100;
        constexpr int DEFAULT_POOL_VERTICES = 100;

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
    memcpy(pool->cpu_ibuffer.get() + pool->index_size * sizeof(UINT),
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
            Vector4 out = Vector4();

            // Now, for each vertex, I will pull the data I want for my
            // stream and then copy it to the end of my buffer.
            for (int j = 0; j < job.vertex_data.size(); j++)
            {
                job.vertex_data[j].pullVertexAttribute((VertexDataStream)i,
                                                       out);

                // Also copy to my CPU-side copy of the data
                memcpy(pool->cpu_vbuffers[i].get() +
                           (pool->vertex_size + j) * byte_size,
                       &out, byte_size);
            }
        }
    }

    // Create my mesh
    const std::shared_ptr<Geometry>& mesh = job.mesh;
    pool->meshes.emplace_back(mesh);

    mesh->indexBuffer = pool->ibuffer;
    for (int i = 0; i < BINDABLE_STREAM_COUNT; i++)
    {
        if (job.layout.hasVertexStream((VertexDataStream)i))
        {
            mesh->vertexBuffers[i] = pool->vbuffers[i];
        }
    }
    mesh->vertexOffset = pool->vertex_size;
    mesh->indexOffset = pool->index_size;
    mesh->indexCount = job.index_data.size() * 3;
    mesh->vertexCount = job.vertex_data.size();

    // Update my mesh pool
    pool->vertex_size += job.vertex_data.size();
    pool->index_size += job.index_data.size() * 3;
}

// System Resources
void ResourceManagerImpl::LoadCubeMesh()
{
    MeshBuilder builder = MeshBuilder();
    builder.addLayout(PosXYZ_TexU);
    builder.addCube(Vector3(0, 0, 0), Quaternion(), 1.f);
    cubeMesh = requestMesh(builder);
}

void ResourceManagerImpl::LoadFallbackColormap()
{
    TextureColor color;
    color.data[0] = 122;
    color.data[1] = 122;
    color.data[2] = 122;
    color.data[3] = 122;
    fallbackColormap = device->createTexture(
        "Fallback Colormap", TextureLayout::R8G8B8A8_UNORM,
        TextureUsage::ShaderResource, 1, 1, 1, 1, false, &color);
}

} // namespace Graphics
} // namespace Engine