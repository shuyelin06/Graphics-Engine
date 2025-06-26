#pragma once

#include <string>
#include <vector>

#include "AssetBuilder.h"
#include "TextureBuilder.h"

struct cgltf_accessor;
struct cgltf_material;
struct cgltf_texture;

namespace Engine {
namespace Graphics {
// GLTFFile:
// Interface for reading GLTF Binary files.
class GLTFFile {
  private:
    std::string path;

  public:
    GLTFFile(const std::string& path);

    Asset* readFromFile(MeshBuilder& mesh_builder, AtlasBuilder& tex_builder,
                        ID3D11Device* device, ID3D11DeviceContext* context);

  private:
    // Material Parsing
    void parseMaterial(const cgltf_material* mat_data,
                       MeshBuilder& mesh_builder, Material& material,
                       AtlasBuilder& tex_builder);
    const AtlasAllocation& parseBaseColorTex(const cgltf_texture* tex,
                                             AtlasBuilder& tex_builder);
};

} // namespace Graphics
} // namespace Engine