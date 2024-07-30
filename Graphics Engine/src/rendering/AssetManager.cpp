#include "AssetManager.h"

#include <fstream>
#include <iostream>
#include <stdio.h>

#include <map>
#include <vector>
#include <string.h>

#include <assert.h>

#include "datamodel/Terrain.h"
#include "math/Vector3.h"
#include "math/Vector2.h"

using namespace std;

namespace Engine
{
using namespace Math;

namespace Graphics
{
	// Constructor and Destructor
    AssetManager::AssetManager() = default;
    AssetManager::~AssetManager() = default;

    // Initialize:
    // Loads assets into the asset manager.
    void AssetManager::initialize()
    {
        assets.resize(AssetCount);

        assets[Cube] = LoadCube();
        // Fox by Jake Blakeley [CC-BY] via Poly Pizza
        assets[Fox] = LoadAssetFromOBJ("data/", "model.obj", "Model");

        Datamodel::Terrain terrain = Datamodel::Terrain();
        terrain.generateMesh();
        assets[Terrain] = terrain.getMesh();
    }

    // GetAssets:
    // Return an asset by name.
    Asset* AssetManager::getAsset(AssetSlot asset)
    {
        assert(0 <= asset && asset < assets.size());
        return assets[asset];
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
		std::ifstream file(path + objFile);
		std::string parsedLine;

		// List of (indexed) vertex positions, textures and normals
		OBJData data;
        data.asset = new Asset();

        // Active mesh
        Mesh* activeMesh = nullptr;

        // Maps "_/_/_" index strings to vertex indices
        // for the active mesh
        std::map<std::string, int> vertexMap = std::map<std::string, int>();

		// Read each line
		while (getline(file, parsedLine))
		{
			// Convert line to a C-format string for use in string functions
			char* line = &parsedLine[0];

			// Ignore empty lines
			if (strlen(line) == 0)
				continue;
            
            // Examine the first token of the line to determine what to do.
            char* token = ParseToken(&line, " ");

            // #: Comment; Ignore
            if (strcmp(token, "#") == 0)
                continue;
            // Material Data (mtllib): Load materials
            else if (strcmp(token, "mtllib") == 0)
            {
                char* matFile = ParseToken(&line, " ");
                ParseMaterials(path, matFile, data);
            }
            // Vertex Data (v): Load x,y,z position
            else if (strcmp(token, "v") == 0)
            {
                Vector3 position;
                position.x = ParseFloat(&line, " ");
                position.y = ParseFloat(&line, " ");
                position.z = ParseFloat(&line, " ");
                data.positions.push_back(position);
            }
            // Texture Data (vt): u,v texture coordinate
            else if (strcmp(token, "vt") == 0)
            {
                Vector2 texture_coord;
                texture_coord.u = ParseFloat(&line, " ");
                texture_coord.v = ParseFloat(&line, " ");
                data.textureCoords.push_back(texture_coord);
            }
            // Normal Data (vn): x,y,z normal
            else if (strcmp(token, "vn") == 0)
            {
                Vector3 normal;
                normal.x = ParseFloat(&line, " ");
                normal.y = ParseFloat(&line, " ");
                normal.z = ParseFloat(&line, " ");
                data.normals.push_back(normal);
            }
            // Use Material (usemtl): Create mesh w/ given material
            else if (strcmp(token, "usemtl") == 0)
            {
                // Finish active mesh by locking it
                if (activeMesh != nullptr)
                    activeMesh->lockMesh(true);

                // Retrieve material
                char* materialName = ParseToken(&line, " ");
                assert(data.materialMap.contains(materialName));
                Material* material = data.materialMap[materialName];

                // Create mesh and assign the material
                activeMesh = data.asset->newMesh();
                activeMesh->setMaterial(material);

                // Clear vertex map
                vertexMap.clear();
            }
            // Face Data (f): Register vertex data under the mesh
            else if (strcmp(token, "f") == 0)
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

                // Read each vertex in the face
                while (strlen(line) > 0)
                {
                    char* vertexData = ParseToken(&line, " ");
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
                        int vIndex = ParseUInt(&vertexData, "/") - 1;
                        assert(vIndex >= 0); // Position should ALWAYS be given.
                        newVertex.position = data.positions[vIndex];

                        // Parse texture index
                        int vtIndex = ParseUInt(&vertexData, "/") - 1;
                        if (vtIndex < 0)
                            newVertex.textureCoord = Vector2(-1, -1);
                        else
                            newVertex.textureCoord = data.textureCoords[vtIndex];

                        // Parse normal index
                        int vnIndex = ParseUInt(&vertexData, "/") - 1;
                        if (vnIndex < 0)
                            newVertex.normal = Vector3(0, 0, 0);
                        else
                            newVertex.normal = data.normals[vnIndex];
                        
                        activeMesh->addVertex(newVertex);
                    }

                    // Log index
                    indices.push_back(vertexIndex);
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

        file.close();

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
        std::ifstream file(path + material_file);
        std::string parsedLine;

        // Current Material
        Material* activeMaterial = nullptr;

        // Read each line
        while (getline(file, parsedLine))
        {
            // Convert line to a C-format string for use in string functions
            char* line = &parsedLine[0];

            // Ignore empty lines
            if (strlen(line) == 0)
                continue;

            // Examine the first token of the line to determine what to do.
            char* token = ParseToken(&line, " ");

            // #: Comment; Ignore
            if (strcmp(token, "#") == 0)
                continue;
            else if (strcmp(token, "newmtl") == 0)
            {
                // Get the material name
                char* matName = ParseToken(&line, " ");

                // Create a new material and register it to its name.
                // Set it as the active material to work on.
                activeMaterial = data.asset->newMaterial();
                data.materialMap[std::string(matName)] = activeMaterial;
            }
            else if (strcmp(token, "Ka") == 0)
            {
                assert(activeMaterial != nullptr);
                activeMaterial->ka.x = ParseFloat(&line, " ");
                activeMaterial->ka.y = ParseFloat(&line, " ");
                activeMaterial->ka.z = ParseFloat(&line, " ");
            }
            else if (strcmp(token, "Kd") == 0)
            {
                assert(activeMaterial != nullptr);
                activeMaterial->kd.x = ParseFloat(&line, " ");
                activeMaterial->kd.y = ParseFloat(&line, " ");
                activeMaterial->kd.z = ParseFloat(&line, " ");
            }
            else if (strcmp(token, "Ks") == 0)
            {
                assert(activeMaterial != nullptr);
                activeMaterial->ks.x = ParseFloat(&line, " ");
                activeMaterial->ks.y = ParseFloat(&line, " ");
                activeMaterial->ks.z = ParseFloat(&line, " ");
            }
            else
                std::cout << token << "\n";
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