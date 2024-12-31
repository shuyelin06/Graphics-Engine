#include "AssetManager.h"

#include <fstream>
#include <iostream>
#include <stdio.h>

#include <map>
#include <string.h>
#include <vector>

#include <assert.h>

#include "datamodel/Terrain.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "rendering/components/AssetBuilder.h"
#include "utility/FileReader.h"

#include "math/PerlinNoise.h"

using namespace std;

namespace Engine {
using namespace Math;
using namespace Utility;
namespace Graphics {
AssetManager::AssetManager(ID3D11Device* _device,
                           ID3D11DeviceContext* _context) {
    device = _device;
    context = _context;

    for (int i = 0; i < CHUNK_X_LIMIT; i++) {
        for (int j = 0; j < CHUNK_Z_LIMIT; j++) {
            terrain_meshes[i][j] = nullptr;
        }
    }
}
AssetManager::~AssetManager() = default;

// Initialize:
// Loads assets into the asset manager.
void AssetManager::initialize() {
    // Create my textures
    textures.resize(TextureCount);
    TextureBuilder tex_builder = TextureBuilder(device, 10, 10);

    textures[Test] = tex_builder.generate();

    //// Noise
    // tex_builder.reset(1000, 1000);
    // for (int i = 1; i <= 1000; i++) {
    //     for (int j = 1; j <= 1000; j++) {
    //         float val =
    //             PerlinNoise::noise2D(i * 4.f / 1087.f, j * 4.f / 1087.f);
    //         assert(0 <= val && val <= 1);
    //         unsigned char convert = (int)(255 * val);
    //         tex_builder.setColor(i - 1, j - 1,
    //                              {convert, convert, convert, 255});
    //     }
    // }
    textures[Perlin] = tex_builder.generate();
    // WriteTextureToPNG(textures[Perlin]->texture, "perlin.png");

    LoadTextureFromPNG(tex_builder, "data/", "test.png");
    textures[Test2] = tex_builder.generate();

    LoadTextureFromPNG(tex_builder, "data/", "grass.png");
    textures[TerrainGrass] = tex_builder.generate();

    // Create my samplers
    samplers.resize(SamplerCount);

    samplers[ShadowMap] = LoadShadowMapSampler();
    samplers[MeshTexture] = LoadMeshTextureSampler();

    // Create my assets
    assets.resize(AssetCount);

    MeshBuilder mesh_builder = MeshBuilder(device);

    assets[Cube] = LoadCube(mesh_builder);
    // Fox by Jake Blakeley [CC-BY] via Poly Pizza
    assets[Fox] = LoadAssetFromOBJ(mesh_builder, "data/", "model.obj", "Model");
}

// GetAsset:
// Return an asset by name.
Asset* AssetManager::getAsset(AssetSlot asset) {
    assert(0 <= asset && asset < assets.size());
    return assets[asset];
}

Texture* AssetManager::getTexture(TextureSlot texture) {
    assert(0 <= texture && texture <= textures.size());
    return textures[texture];
}

ID3D11SamplerState* AssetManager::getSampler(SamplerSlot sampler) {
    assert(0 <= sampler && sampler <= samplers.size());
    return samplers[sampler];
}

// GetTerrain:
// Generates terrain given data.
Mesh* AssetManager::getTerrainMesh(int x, int z, TerrainData data) {
    if (terrain_meshes[x][z] == nullptr) {
        MeshBuilder builder = MeshBuilder(device);
        terrain_meshes[x][z] = GenerateTerrainMesh(builder, data);
    }

    return terrain_meshes[x][z];
}

// LoadMeshFromOBJ
// Implements a simple OBJ file parser to load an asset.
// Meshes can only have one material; so an obj file with multiple materials
// will generate multiple meshes.
struct OBJData {
    // Asset to Generate
    Asset* asset;

    // Vertex Descriptions
    std::vector<Vector3> positions;
    std::vector<Vector2> textureCoords;
    std::vector<Vector3> normals;

    // Maps Strings to Material Indices
    std::map<std::string, Material*> materialMap;
};

// Helper parsing functions
static char* ParseToken(char** line, const char* delimiter);
static int ParseUInt(char** line, const char* delimiter);
static float ParseFloat(char** line, const char* delimiter);

static void ParseMaterials(std::string path, std::string material_file,
                           OBJData& data);

Asset* AssetManager::LoadAssetFromOBJ(MeshBuilder& builder, std::string path,
                                      std::string objFile,
                                      std::string assetName) {
    // Open target file with file reader
    TextFileReader fileReader = TextFileReader(path + objFile);

    // List of (indexed) vertex positions, textures and normals
    OBJData data;

    data.asset = new Asset();
    builder.reset();

    // Maps "_/_/_" index strings to vertex indices
    // for the active mesh
    std::map<std::string, int> vertexMap = std::map<std::string, int>();

    // Read each line
    while (fileReader.extractBlock('\n')) {
        // Ignore empty lines
        if (fileReader.viewBlock().length() != 0) {
            std::string token;
            fileReader.readString(&token, ' ');

            // #: Comment; Ignore
            // Material Data (mtllib): Load materials
            if (token == "mtllib") {
                std::string matFile;
                fileReader.readString(&matFile, ' ');
                ParseMaterials(path, matFile, data);
            }
            // Vertex Data (v): Load x,y,z position
            else if (token == "v") {
                Vector3 position;
                fileReader.readFloat(&position.x, ' ');
                fileReader.readFloat(&position.y, ' ');
                fileReader.readFloat(&position.z, ' ');
                data.positions.push_back(position);
            }
            // Texture Data (vt): u,v texture coordinate
            else if (token == "vt") {
                Vector2 texture_coord;
                fileReader.readFloat(&texture_coord.x, ' ');
                fileReader.readFloat(&texture_coord.y, ' ');
                data.textureCoords.push_back(texture_coord);
            }
            // Normal Data (vn): x,y,z normal
            else if (token == "vn") {
                Vector3 normal;
                fileReader.readFloat(&normal.x, ' ');
                fileReader.readFloat(&normal.y, ' ');
                fileReader.readFloat(&normal.z, ' ');
                data.normals.push_back(normal);
            }
            // Use Material (usemtl): Create mesh w/ given material
            else if (token == "usemtl") {
                // Create a mesh from the builder
                Mesh* newMesh = builder.generate();

                if (newMesh != nullptr) {
                    data.asset->addMesh(newMesh);

                    // Retrieve material
                    std::string materialName;

                    fileReader.readString(&materialName, ' ');
                    assert(data.materialMap.contains(materialName));

                    Material* material = data.materialMap[materialName];
                    newMesh->material = material;

                    // Clear vertex map
                    vertexMap.clear();
                }
            }
            // Face Data (f): Register vertex data under the mesh
            else if (token == "f") {
                // Stores the indices for this face. If there are >3 vertices,
                // we'll have to triangulate the face.
                std::vector<int> indices;

                // Read each vertex _/_/_ in the face
                while (fileReader.extractBlock(' ')) {
                    const std::string vertexData = fileReader.viewBlock();
                    int vertexIndex = -1;

                    // Check if the _/_/_ vertex combination exists.
                    // If it does, use the pre-existing index.
                    if (vertexMap.contains(vertexData))
                        vertexIndex = vertexMap[vertexData];
                    // Otherwise, create a new vertex in the vertex buffer
                    // and register the combination in the vertex_map.
                    else {
                        // Parse the _/_/_ vertex combination. Note that OBJ
                        // files are 1-indexed, whereas our vectors are
                        // 0-indexed.
                        MeshVertex newVertex = MeshVertex();

                        // Position
                        int vIndex;
                        if (fileReader.readInt(&vIndex, '/'))
                            newVertex.position = data.positions[vIndex - 1];
                        else // Position should ALWAYS be given.
                            assert(false);

                        // Parse texture index
                        int vtIndex;
                        if (fileReader.readInt(&vtIndex, '/'))
                            newVertex.textureCoord =
                                data.textureCoords[vtIndex - 1];
                        else
                            newVertex.textureCoord = Vector2(-1, -1);

                        // Parse normal index
                        int vnIndex;
                        if (fileReader.readInt(&vnIndex, '/'))
                            newVertex.normal = data.normals[vnIndex - 1];
                        else
                            newVertex.normal = Vector3(0, 0, 0);

                        vertexIndex = builder.addVertex(newVertex.position,
                                                        newVertex.textureCoord,
                                                        newVertex.normal);
                        vertexMap[vertexData] = vertexIndex;
                    }

                    // Log index
                    indices.push_back(vertexIndex);

                    fileReader.popBlock();
                }

                assert(indices.size() >= 3);

                // We now parse the indices of the face into individual
                // triangles. If we have an N-gon formed by indices 0, 1, 2, 3,
                // 4, 5, we can triangulate it as [0,1,2], [0,2,3], [0,3,4],
                // [0,4,5].
                for (int i = 2; i < indices.size(); i++) {
                    MeshTriangle triangle =
                        MeshTriangle(indices[0], indices[i - 1], indices[i]);
                    builder.addTriangle(triangle.vertex0, triangle.vertex1,
                                        triangle.vertex2);
                }
            }
            // Print if unhandled
            else {
                std::cout << token << "\n";
                // assert(false);
            }
        }

        fileReader.popBlock();
    }

    // Finish active mesh by locking it
    Mesh* newMesh = builder.generate();

    if (newMesh != nullptr) {
        data.asset->addMesh(newMesh);
    }

    return data.asset;
}

// Parses a material (or materials), and registers them under the asset.
// For each property of a material, the last property given will be the one
// used.
void ParseMaterials(std::string path, std::string material_file,
                    OBJData& data) {
    // Open target file with file reader
    TextFileReader fileReader = TextFileReader(path + material_file);

    // Current Material
    Material* activeMaterial = nullptr;

    // Read each line
    while (fileReader.extractBlock('\n')) {
        // Ignore empty lines
        if (fileReader.viewBlock().length() != 0) {
            fileReader.lstripBlock(' ', LSTRIP_INFINITE);

            // Examine the first token of the line to determine what to do.
            std::string token;
            fileReader.readString(&token, ' ');

            // #: Comment; Ignore
            if (token == "newmtl") {
                // Get the material name
                std::string matName;
                fileReader.readString(&matName, ' ');

                // Create a new material and register it to its name.
                // Set it as the active material to work on.
                Material* temp = new Material();
                data.asset->addMaterial(temp);
                activeMaterial = temp;

                data.materialMap[matName] = activeMaterial;
            } else if (token == "Ka") {
                assert(activeMaterial != nullptr);
                fileReader.readFloat(&activeMaterial->ka.r, ' ');
                fileReader.readFloat(&activeMaterial->ka.g, ' ');
                fileReader.readFloat(&activeMaterial->ka.b, ' ');
            } else if (token == "Kd") {
                assert(activeMaterial != nullptr);
                fileReader.readFloat(&activeMaterial->kd.r, ' ');
                fileReader.readFloat(&activeMaterial->kd.g, ' ');
                fileReader.readFloat(&activeMaterial->kd.b, ' ');
            } else if (token == "Ks") {
                assert(activeMaterial != nullptr);
                fileReader.readFloat(&activeMaterial->ks.r, ' ');
                fileReader.readFloat(&activeMaterial->ks.g, ' ');
                fileReader.readFloat(&activeMaterial->ks.b, ' ');
            } else
                std::cout << token << "\n";
        }

        fileReader.popBlock();
    }
}

// Helper parser functions. Given a pointer to a C-string, parses the next token
// (space delimited word) and tries to convert it to the format specified.
char* ParseToken(char** line, const char* delimiter) {
    char* remainder = NULL;

    // Parse the token
    char* token = strtok_s(*line, delimiter, &remainder);
    assert(token != NULL);

    // Update the line
    *line = remainder;

    return token;
}

int ParseUInt(char** line, const char* delimiter) {
    char* remainder = NULL;

    int delimiterLength = strlen(delimiter);

    // Check if the delimiter prefixes the string. If it does, return -1.
    if (strncmp(*line, delimiter, delimiterLength) == 0) {
        int offset = 0;

        while (*(*line + delimiterLength + offset) != NULL) {
            (*line)[offset] = (*line + delimiterLength)[offset];
            offset += 1;
        }
        (*line)[offset] = (*line + delimiterLength)[offset];

        return -1;
    }
    // Otherwise, use str_tok to find the token before the first delimiter
    else {
        char* token = strtok_s(*line, delimiter, &remainder);
        assert(token != NULL);
        float result = atoi(token);

        // Update the line
        *line = remainder;

        return result;
    }
}

float ParseFloat(char** line, const char* delimiter) {
    char* remainder = NULL;

    // Parse the token
    char* token = strtok_s(*line, delimiter, &remainder);
    assert(token != NULL);
    float result = (float)atof(token);

    // Update the line
    *line = remainder;

    return result;
}

// Hard-Coded Cube Creator
// Used in debugging
Asset* AssetManager::LoadCube(MeshBuilder& builder) {
    // We duplicate vertices so that the cube has sharp normals.
    const MeshVertex vertices[] = {
        // Front Face
        MeshVertex(Vector3(-1, -1, 1), Vector2(), Vector3(0, 0, 1)),
        MeshVertex(Vector3(1, -1, 1), Vector2(), Vector3(0, 0, 1)),
        MeshVertex(Vector3(1, 1, 1), Vector2(), Vector3(0, 0, 1)),
        MeshVertex(Vector3(-1, 1, 1), Vector2(), Vector3(0, 0, 1)),
        // Back Face
        MeshVertex(Vector3(-1, -1, -1), Vector2(), Vector3(0, 0, -1)),
        MeshVertex(Vector3(1, -1, -1), Vector2(), Vector3(0, 0, -1)),
        MeshVertex(Vector3(1, 1, -1), Vector2(), Vector3(0, 0, -1)),
        MeshVertex(Vector3(-1, 1, -1), Vector2(), Vector3(0, 0, -1)),
        // Top Face
        MeshVertex(Vector3(-1, 1, -1), Vector2(), Vector3(0, 1, 0)),
        MeshVertex(Vector3(1, 1, -1), Vector2(), Vector3(0, 1, 0)),
        MeshVertex(Vector3(1, 1, 1), Vector2(), Vector3(0, 1, 0)),
        MeshVertex(Vector3(-1, 1, 1), Vector2(), Vector3(0, 1, 0)),
        // Bottom Face
        MeshVertex(Vector3(-1, -1, -1), Vector2(), Vector3(0, -1, 0)),
        MeshVertex(Vector3(1, -1, -1), Vector2(), Vector3(0, -1, 0)),
        MeshVertex(Vector3(1, -1, 1), Vector2(), Vector3(0, -1, 0)),
        MeshVertex(Vector3(-1, -1, 1), Vector2(), Vector3(0, -1, 0)),
        // Right Face
        MeshVertex(Vector3(1, -1, -1), Vector2(), Vector3(1, 0, 0)),
        MeshVertex(Vector3(1, 1, -1), Vector2(), Vector3(1, 0, 0)),
        MeshVertex(Vector3(1, 1, 1), Vector2(), Vector3(1, 0, 0)),
        MeshVertex(Vector3(1, -1, 1), Vector2(), Vector3(1, 0, 0)),
        // Left Face
        MeshVertex(Vector3(-1, -1, -1), Vector2(), Vector3(-1, 0, 0)),
        MeshVertex(Vector3(-1, 1, -1), Vector2(), Vector3(-1, 0, 0)),
        MeshVertex(Vector3(-1, 1, 1), Vector2(), Vector3(-1, 0, 0)),
        MeshVertex(Vector3(-1, -1, 1), Vector2(), Vector3(-1, 0, 0)),
    };

    const int indices[] = {
        0,  1,  2,  2,  3,  0,  // Front Face
        4,  6,  5,  6,  4,  7,  // Back Face
        8,  10, 9,  10, 8,  11, // Top Face
        12, 13, 14, 14, 15, 12, // Bottom Face
        16, 17, 18, 18, 19, 16, // Right Face
        20, 22, 21, 22, 20, 23  // Left Face
    };

    Asset* cube = new Asset();
    builder.reset();

    for (int i = 0; i < 24; i++) {
        MeshVertex vertex = vertices[i];
        builder.addVertex(vertex.position, vertex.textureCoord, vertex.normal);
    }

    for (int i = 0; i < 12; i++) {
        MeshTriangle triangle = MeshTriangle(indices[i * 3], indices[i * 3 + 1],
                                             indices[i * 3 + 2]);
        builder.addTriangle(triangle.vertex0, triangle.vertex1,
                            triangle.vertex2);
    }

    cube->addMesh(builder.generate());

    return cube;
}

// Load___Sampler:
// Create Texture Samplers!
ID3D11SamplerState* AssetManager::LoadShadowMapSampler() {
    ID3D11SamplerState* sampler;

    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.BorderColor[0] = 0.f;
    sampler_desc.BorderColor[1] = 0.f;
    sampler_desc.BorderColor[2] = 0.f;
    sampler_desc.BorderColor[3] = 0.f;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = 1.0f;

    device->CreateSamplerState(&sampler_desc, &sampler);
    assert(sampler != NULL);

    return sampler;
}

ID3D11SamplerState* AssetManager::LoadMeshTextureSampler() {
    ID3D11SamplerState* sampler;

    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    /*sampler_desc.BorderColor[0] = 0.f;
    sampler_desc.BorderColor[1] = 0.f;
    sampler_desc.BorderColor[2] = 0.f;
    sampler_desc.BorderColor[3] = 0.f;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = 1.0f;*/

    device->CreateSamplerState(&sampler_desc, &sampler);
    assert(sampler != NULL);

    return sampler;
}
} // namespace Graphics
} // namespace Engine