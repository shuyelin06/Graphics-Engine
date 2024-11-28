#include "AssetManager.h"

#include <fstream>
#include <iostream>
#include <stdio.h>

#include <map>
#include <vector>
#include <string.h>

#include <assert.h>

#include "rendering/components/AssetBuilder.h"
#include "utility/FileReader.h"
#include "datamodel/Terrain.h"
#include "math/Vector3.h"
#include "math/Vector2.h"

using namespace std;

namespace Engine
{
using namespace Math;
using namespace Utility;
namespace Graphics
{
    AssetWrapper::AssetWrapper(Asset* _asset, ID3D11Device* device)
    {
        asset = _asset;
        loader = new AssetLoader(_asset, device);
    }

	// Constructor and Destructor
    AssetWrapper::AssetWrapper() = default;
    AssetManager::AssetManager(ID3D11Device* _device)
    {
        device = _device;
    }
    AssetManager::~AssetManager() = default;

    // Initialize:
    // Loads assets into the asset manager.
    void AssetManager::initialize()
    {
        assets.resize(AssetCount);
        textures.resize(TextureCount);

        TextureBuilder tex_builder = TextureBuilder(1,1);
        
        textures[Test] = tex_builder.generate(device);
        LoadTextureFromPNG(tex_builder, "data/", "test.png");
        textures[Test2] = tex_builder.generate(device);

        assets[Cube] = AssetWrapper(LoadCube(), device);
        // Fox by Jake Blakeley [CC-BY] via Poly Pizza
        assets[Fox] = AssetWrapper(LoadAssetFromOBJ("data/", "model.obj", "Model"), device);

        Datamodel::Terrain terrain = Datamodel::Terrain();
        terrain.generateMesh();
        assets[Terrain] = AssetWrapper(terrain.getMesh(), device);

        
    }

    // GetAsset:
    // Return an asset by name.
    Asset* AssetManager::getAsset(AssetSlot asset)
    {
        assert(0 <= asset && asset < assets.size());
        return assets[asset].asset;
    }

    // GetAssetLoader:
    // Return an asset's loader by name
    AssetLoader* AssetManager::getAssetLoader(AssetSlot asset)
    {
        assert(0 <= asset && asset < assets.size());
        return assets[asset].loader;
    }
    
    Texture* AssetManager::getTexture(TextureSlot texture)
    {
        assert(0 <= texture && texture <= textures.size());
        return textures[texture];
    }

	// LoadMeshFromOBJ
	// Implements a simple OBJ file parser to load an asset.
    // Meshes can only have one material; so an obj file with multiple materials
    // will generate multiple meshes. 
    struct OBJData
    {
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

    static void ParseMaterials(std::string path, std::string material_file, OBJData& data);

    Asset* AssetManager::LoadAssetFromOBJ(std::string path, std::string objFile, std::string assetName)
	{
		// Open target file with file reader
        TextFileReader fileReader = TextFileReader(path + objFile);
		
		// List of (indexed) vertex positions, textures and normals
		OBJData data;
        data.asset = new Asset();

        // Active mesh
        Mesh* activeMesh = nullptr;

        // Maps "_/_/_" index strings to vertex indices
        // for the active mesh
        std::map<std::string, int> vertexMap = std::map<std::string, int>();

		// Read each line
		while (fileReader.extractBlock('\n'))
		{
			// Ignore empty lines
            if (fileReader.viewBlock().length() != 0)
            {
                std::string token;
                fileReader.readString(&token, ' ');

                // #: Comment; Ignore
                // Material Data (mtllib): Load materials
                if (token == "mtllib")
                {
                    std::string matFile;
                    fileReader.readString(&matFile, ' ');
                    ParseMaterials(path, matFile, data);
                }
                // Vertex Data (v): Load x,y,z position
                else if (token == "v")
                {
                    Vector3 position;
                    fileReader.readFloat(&position.x, ' ');
                    fileReader.readFloat(&position.y, ' ');
                    fileReader.readFloat(&position.z, ' ');
                    data.positions.push_back(position);
                }
                // Texture Data (vt): u,v texture coordinate
                else if (token == "vt")
                {
                    Vector2 texture_coord;
                    fileReader.readFloat(&texture_coord.x, ' ');
                    fileReader.readFloat(&texture_coord.y, ' ');
                    data.textureCoords.push_back(texture_coord);
                }
                // Normal Data (vn): x,y,z normal
                else if (token == "vn")
                {
                    Vector3 normal;
                    fileReader.readFloat(&normal.x, ' ');
                    fileReader.readFloat(&normal.y, ' ');
                    fileReader.readFloat(&normal.z, ' ');
                    data.normals.push_back(normal);
                }
                // Use Material (usemtl): Create mesh w/ given material
                else if (token == "usemtl")
                {
                    // Finish active mesh by locking it
                    if (activeMesh != nullptr)
                        activeMesh->lockMesh(true);

                    // Retrieve material
                    std::string materialName;
                    fileReader.readString(&materialName, ' ');
                    assert(data.materialMap.contains(materialName));
                    Material* material = data.materialMap[materialName];

                    // Create mesh and assign the material
                    activeMesh = data.asset->newMesh();
                    activeMesh->setMaterial(material);

                    // Clear vertex map
                    vertexMap.clear();
                }
                // Face Data (f): Register vertex data under the mesh
                else if (token == "f")
                {
                    // If there is no active mesh, create one.
                    if (activeMesh == nullptr)
                    {
                        activeMesh = data.asset->newMesh();

                        // Clear vertex map
                        vertexMap.clear();
                    }

                    // Stores the indices for this face. If there are >3 vertices, we'll have to
                    // triangulate the face.
                    std::vector<int> indices;

                    // Read each vertex _/_/_ in the face
                    while (fileReader.extractBlock(' '))
                    {
                        const std::string vertexData = fileReader.viewBlock();
                        int vertexIndex = -1;

                        // Check if the _/_/_ vertex combination exists. 
                        // If it does, use the pre-existing index. 
                        if (vertexMap.contains(vertexData))
                            vertexIndex = vertexMap[vertexData];
                        // Otherwise, create a new vertex in the vertex buffer
                        // and register the combination in the vertex_map.
                        else {
                            vertexIndex = activeMesh->vertexCount();
                            vertexMap[vertexData] = vertexIndex;

                            // Parse the _/_/_ vertex combination. Note that OBJ files
                            // are 1-indexed, whereas our vectors are 0-indexed.
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
                                newVertex.textureCoord = data.textureCoords[vtIndex - 1];
                            else
                                newVertex.textureCoord = Vector2(-1, -1);    

                            // Parse normal index
                            int vnIndex;
                            if (fileReader.readInt(&vnIndex, '/'))
                                newVertex.normal = data.normals[vnIndex - 1];
                            else
                                newVertex.normal = Vector3(0, 0, 0);

                            activeMesh->addVertex(newVertex);
                        }

                        // Log index
                        indices.push_back(vertexIndex);
                        
                        fileReader.popBlock();
                    }

                    assert(indices.size() >= 3);

                    // We now parse the indices of the face into individual triangles.
                    // If we have an N-gon formed by indices 0, 1, 2, 3, 4, 5,
                    // we can triangulate it as [0,1,2], [0,2,3], [0,3,4], [0,4,5].
                    for (int i = 2; i < indices.size(); i++)
                    {
                        MeshTriangle triangle = MeshTriangle(indices[0], indices[i - 1], indices[i]);
                        activeMesh->addTriangle(triangle);
                    }
                }
                // Print if unhandled
                else
                {
                    std::cout << token << "\n";
                    // assert(false);
                }
            }

            fileReader.popBlock();
		}

        // Finish active mesh by locking it
        if (activeMesh != nullptr)
            activeMesh->lockMesh(true);

        return data.asset;
	}

    // Parses a material (or materials), and registers them under the asset.
    // For each property of a material, the last property given will be the one used.
    void ParseMaterials(std::string path, std::string material_file, OBJData& data)
    {
        // Open target file with file reader
        TextFileReader fileReader = TextFileReader(path + material_file);

        // Current Material
        Material* activeMaterial = nullptr;

        // Read each line
        while (fileReader.extractBlock('\n'))
        {
            // Ignore empty lines
            if (fileReader.viewBlock().length() != 0)
            {
                fileReader.lstripBlock(' ', LSTRIP_INFINITE);

                // Examine the first token of the line to determine what to do.
                std::string token;
                fileReader.readString(&token, ' ');

                // #: Comment; Ignore
                if (token == "newmtl")
                {
                    // Get the material name
                    std::string matName;
                    fileReader.readString(&matName, ' ');

                    // Create a new material and register it to its name.
                    // Set it as the active material to work on.
                    activeMaterial = data.asset->newMaterial();
                    data.materialMap[matName] = activeMaterial;
                }
                else if (token == "Ka")
                {
                    assert(activeMaterial != nullptr);
                    fileReader.readFloat(&activeMaterial->ka.r, ' ');
                    fileReader.readFloat(&activeMaterial->ka.g, ' ');
                    fileReader.readFloat(&activeMaterial->ka.b, ' ');
                }
                else if (token == "Kd")
                {
                    assert(activeMaterial != nullptr);
                    fileReader.readFloat(&activeMaterial->kd.r, ' ');
                    fileReader.readFloat(&activeMaterial->kd.g, ' ');
                    fileReader.readFloat(&activeMaterial->kd.b, ' ');
                }
                else if (token == "Ks")
                {
                    assert(activeMaterial != nullptr);
                    fileReader.readFloat(&activeMaterial->ks.r, ' ');
                    fileReader.readFloat(&activeMaterial->ks.g, ' ');
                    fileReader.readFloat(&activeMaterial->ks.b, ' ');
                }
                else
                    std::cout << token << "\n";
            }

            fileReader.popBlock();
        }
    }

    // Helper parser functions. Given a pointer to a C-string, parses the next token (space
    // delimited word) and tries to convert it to the format specified.
    char* ParseToken(char** line, const char* delimiter)
    {
        char* remainder = NULL;

        // Parse the token 
        char* token = strtok_s(*line, delimiter, &remainder);
        assert(token != NULL);

        // Update the line
        *line = remainder;

        return token;
    }

    int ParseUInt(char** line, const char* delimiter)
    {
        char* remainder = NULL;

        int delimiterLength = strlen(delimiter);

        // Check if the delimiter prefixes the string. If it does, return -1.
        if (strncmp(*line, delimiter, delimiterLength) == 0)
        {
            int offset = 0;

            while (*(*line + delimiterLength + offset) != NULL)
            {
                (*line)[offset] = (*line + delimiterLength)[offset];
                offset += 1;
            } 
            (*line)[offset] = (*line + delimiterLength)[offset];
            
            return -1;
        }
        // Otherwise, use str_tok to find the token before the first delimiter
        else
        {
            char* token = strtok_s(*line, delimiter, &remainder);
            assert(token != NULL);
            float result = atoi(token);

            // Update the line
            *line = remainder;

            return result;
        }
    }

    float ParseFloat(char** line, const char* delimiter)
    {
        char* remainder = NULL;

        // Parse the token 
        char* token = strtok_s(*line, delimiter, &remainder);
        assert(token != NULL);
        float result = (float) atof(token);

        // Update the line
        *line = remainder;

        return result;
    }

    // Hard-Coded Cube Creator
    // Used in debugging
    Asset* AssetManager::LoadCube()
    {
        // We duplicate vertices so that the cube has sharp normals.
        const MeshVertex vertices[] =
        {
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

        const int indices[] =
        {
            0, 1, 2,  2, 3, 0,   // Front Face
            4, 6, 5,  6, 4, 7,   // Back Face
            8, 10, 9,  10, 8, 11, // Top Face
            12, 13, 14,  14, 15, 12, // Bottom Face
            16, 17, 18,  18, 19, 16, // Right Face
            20, 22, 21,  22, 20, 23  // Left Face
        };

        Asset* cube = new Asset();
        Mesh* mesh = cube->newMesh();

        for (int i = 0; i < 24; i++)
        {
            MeshVertex vertex = vertices[i];
            mesh->addVertex(vertex);
        }

        for (int i = 0; i < 12; i++)
        {
            MeshTriangle triangle = MeshTriangle(indices[i * 3], indices[i * 3 + 1], indices[i * 3 + 2]);
            mesh->addTriangle(triangle);
        }

        mesh->lockMesh(false);

        return cube; 
    }
}
}