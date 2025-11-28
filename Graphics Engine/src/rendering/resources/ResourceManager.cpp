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
    // Empirical testing has shown that
    // 300,000 vertices, 200,000 indices is enough
    // TODO: Be able to create mesh pools on demand
    mesh_pools[MeshPoolType_Terrain] = std::make_unique<MeshPool>(
        (1 << POSITION) | (1 << NORMAL), 800000, 600000);
    mesh_pools[MeshPoolType_Default] =
        std::make_unique<MeshPool>(0xFFFF, 100000, 100000);

    // System assets are loaded here
    LoadCubeMesh();
    LoadFallbackColormap();

    mesh_pools[MeshPoolType_Default]->createGPUResources(device);
    mesh_pools[MeshPoolType_Default]->updateGPUResources(context);
}

// Get Resources
std::shared_ptr<Mesh> ResourceManager::getMesh(int index) const {
    assert(0 <= index && index < meshes.size());
    return meshes[index];
}
std::shared_ptr<Texture> ResourceManager::getTexture(int index) const {
    assert(0 <= index && index < textures.size());
    return textures[index];
}
std::shared_ptr<Geometry> ResourceManager::getGeometry(int index) const {
    assert(0 <= index && index < geometries.size());
    return geometries[index];
}

// CreateGeometry:
// Interface for creating geometry for the pipeline
std::shared_ptr<Geometry>
ResourceManager::CreateGeometry(const GeometryDesc& desc) {
    std::shared_ptr<Geometry> geometry = std::make_shared<Geometry>();
    geometry->mesh = std::move(desc.mesh);
    geometry->material = desc.material;
    geometries.emplace_back(std::move(geometry));
    return geometries.back();
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
        std::shared_ptr<MeshBuilder> builder =
            createMeshBuilder(MeshPoolType_Default);
        GLTFFile::ReadGLTFMesh(full_path, *builder);
        output = builder->generateMesh(context);
    } else
        assert(false); // Unsupported Format

    if (output != nullptr) {
        meshes.emplace_back(std::move(output));
        return meshes.back();
    } else
        return nullptr;
}

std::shared_ptr<MeshBuilder>
ResourceManager::createMeshBuilder(MeshPoolType pool_type) {
    return std::make_shared<MeshBuilder>(mesh_pools[pool_type].get());
}

std::shared_ptr<TextureBuilder> ResourceManager::createTextureBuilder() {
    return std::make_shared<TextureBuilder>(1, 1);
}

MeshPool* ResourceManager::getMeshPool(MeshPoolType pool_type) {
    return mesh_pools[pool_type].get();
}

// Debug Display
void ResourceManager::imGui() {
    if (ImGui::CollapsingHeader("Resource Manager")) {
        ImGui::SeparatorText("Terrain Mesh Pool");
        ImGui::Indent();
        {
            ImGui::Text("Allocations: %zu",
                        mesh_pools[MeshPoolType_Terrain]->meshes.size());
            ImGui::Text("Vertex Count: %u",
                        mesh_pools[MeshPoolType_Terrain]->vertex_size);
            ImGui::Text("Triangle Count: %u",
                        mesh_pools[MeshPoolType_Terrain]->triangle_size);
        }
        ImGui::Unindent();

        ImGui::Text("Mesh Count: %zu", meshes.size());
        if (ImGui::BeginTable("Mesh Information", 3)) {
            ImGui::TableSetupColumn("Index");
            ImGui::TableSetupColumn("Vertex Count");
            ImGui::TableSetupColumn("Index Count");
            ImGui::TableHeadersRow();

            int mesh_index = 0;
            for (const auto& mesh : meshes) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%i", mesh_index++);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", mesh->num_triangles * 3);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%zu", mesh->num_vertices);
            }

            ImGui::EndTable();
        }

        ImGui::Text("Texture Count: %zu", textures.size());
    }
}

// WriteTextureToPNG:
// Uses the PNGFile interface to write a texture to a PNG file
bool ResourceManager::WriteTextureToPNG(ID3D11Texture2D* texture,
                                        std::string path, std::string file) {
    PNGFile png_file = PNGFile(path + file);
    return png_file.writePNGData(device, context, texture);
}

// System Resources
void ResourceManager::LoadCubeMesh() {
    std::shared_ptr<MeshBuilder> builder =
        createMeshBuilder(MeshPoolType_Default);
    builder->addLayout(POSITION);
    builder->addCube(Vector3(0, 0, 0), Quaternion(), 1.f);

    std::shared_ptr<Mesh> mesh = builder->generateMesh(context);
    assert(meshes.size() == SystemMesh_Cube);
    meshes.emplace_back(std::move(mesh));
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