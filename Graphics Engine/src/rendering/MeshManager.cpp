#include "MeshManager.h"

#include <fstream>
#include <iostream>
#include <stdio.h>

#include <map>
#include <vector>
#include <string.h>

#include <assert.h>

#include "math/Vector3.h"
#include "math/Vector2.h"

using namespace std;

namespace Engine
{
using namespace Math;

namespace Graphics
{
	// Stores intermediate data during OBJ parsing
	struct OBJData
	{
		std::vector<Vector3> positions;
		std::vector<Vector2> texture_coords;
		std::vector<Vector3> normals;

        std::vector<float> vertices;
		std::vector<int> indices;

        // Maps "_/_/_" index strings to vertex indices
        std::map<std::string, int> vertex_map;
	};

	// Constructor and Destructor
	MeshManager::MeshManager() = default;
	MeshManager::~MeshManager() = default;

	// GetMesh:
	// Returns the mesh with the stored by the manager. If 
	// no such mesh exists, returns a nullptr.
	Mesh* MeshManager::getMesh(string mesh_name)
	{
		if (meshes.contains(mesh_name))
			return meshes.at(mesh_name);
		else
			return nullptr;
	}

	// LoadMeshFromOBJ
	// Implements a simple OBJ file parser to load a mesh.
	// Assumes that the OBJ file only has one mesh.
	static void Parse_V(char* line, OBJData& data);
	static void Parse_VT(char* line, OBJData& data);
	static void Parse_VN(char* line, OBJData& data);
	static void Parse_F(char* line, OBJData& data);

	void MeshManager::LoadMeshFromOBJ(std::string obj_file, std::string mesh_name)
	{
		// Open target file with file reader
		std::ifstream file(obj_file);
		std::string line;

		// List of (indexed) vertex positions, textures and normals
		OBJData data;

		// Read each line
		while (getline(file, line))
		{
			// Convert line to a C-format string for use in string functions
			char* c_line = &line[0];

			// Ignore empty lines
			if (strlen(c_line) == 0)
				continue;

			// Parse first symbol, telling us what the data in this line is
			// `remainder` stores everything that has yet to be parsed
			char* remainder = NULL;
			char* token = strtok_s(c_line, " ", &remainder);

            // #: Comment; Ignore
            if (strcmp(token, "#") == 0)
                continue;

			// Vertex Data (v): Parse x,y,z Position
            if (strcmp(token, "v") == 0)
                Parse_V(remainder, data);
            // Texture Data (vt): Parse u, v Texture Coordinates
            else if (strcmp(token, "vt") == 0)
                Parse_VT(remainder, data);
            // Normal Data (vn): Parse x,y,z Normals
            else if (strcmp(token, "vn") == 0)
                Parse_VN(remainder, data);
            // Face Data (f): Register a Face
            else if (strcmp(token, "f") == 0)
                Parse_F(remainder, data);
            // Fail if unhandled
            else
                std::cout << token << "\n";
		}

		file.close();
	}

    // Parses a "v" data element to form an x, y, z position.
    // Adds this position to the positions vector once processed.
    static void Parse_V(char* line, OBJData& data)
    {
        Vector3 position;

        char* token = NULL;
        char* remainder = NULL;

        // Parse X
        token = strtok_s(line, " ", &remainder);
        assert(token != NULL);
        position.x = (float) atof(token);

        // Parse Y
        token = strtok_s(NULL, " ", &remainder);
        assert(token != NULL);
        position.y = (float) atof(token);

        // Parse Z
        token = strtok_s(NULL, " ", &remainder);
        assert(token != NULL);
        position.z = (float) atof(token);

        // Check that there is nothing else left
        token = strtok_s(NULL, "", &remainder);
        assert(token == NULL);

        // Push to vector
        data.positions.push_back(position);
    }

    // Parses a "vt" data element for a u, v texture coordinate.
    // Adds this texture coordinate to the texture coordinate
    // vector once processed.
    static void Parse_VT(char* line, OBJData& data)
    {
        Vector2 texture_coord;

        char* token = NULL;
        char* remainder = NULL;

        // Parse u
        token = strtok_s(line, " ", &remainder);
        assert(token != NULL);
        texture_coord.u = (float) atof(token);

        // Parse v
        token = strtok_s(NULL, " ", &remainder);
        assert(token != NULL);
        texture_coord.v = (float) atof(token);

        // Check that there is nothing else left
        token = strtok_s(NULL, "", &remainder);
        assert(token == NULL);

        // Push to vector
        data.texture_coords.push_back(texture_coord);
    }

    // Parses a "vn" data element for a x, y, z normal vector.
    // Adds this normal to the normals vector once processed.
    static void Parse_VN(char* line, OBJData& data)
    {
        Vector3 normal;

        char* token = NULL;
        char* remainder = NULL;

        // Parse X
        token = strtok_s(line, " ", &remainder);
        assert(token != NULL);
        normal.x = (float) atof(token);

        // Parse Y
        token = strtok_s(NULL, " ", &remainder);
        assert(token != NULL);
        normal.y = (float) atof(token);

        // Parse Z
        token = strtok_s(NULL, " ", &remainder);
        assert(token != NULL);
        normal.z = (float) atof(token);

        // Check that there is nothing else left
        token = strtok_s(NULL, "", &remainder);
        assert(token == NULL);

        // Push to vector
        data.normals.push_back(normal);
    }

    // Parses a "f" data element for a face. In the case that the
    // face is an N-gon, splits the face into triangles.
    void Parse_F(char* line, OBJData& data)
    {
        char* token = NULL;
        char* remainder = NULL;

        // List of indices for the face
        std::vector<int> indices;

        // Parse each vertex of the face.
        token = strtok_s(line, " ", &remainder);

        while (token != NULL)
        {
            // Index of the vertex
            int index;

            // Check if the _/_/_ vertex combination exists. 
            // If it does, use the pre-existing index. 
            if (data.vertex_map.contains(token))
                index = data.vertex_map[token];
            // Otherwise, create a new vertex in vertices
            // and register the combination in the vertex_map.
            else {
                index = (int) data.indices.size();
                data.vertex_map[token] = index;

                // Parse the _/_/_ vertex combination. Note that OBJ files
                // are 1-indexed, whereas our vectors are 0-indexed.
                char* f_element = NULL;
                char* f_remainder = NULL;

                // Parse position index
                f_element = strtok_s(token, "/", &f_remainder);
                int v_index = atoi(f_element) - 1;

                Vector3& position = data.positions[v_index];
                data.vertices.push_back(position.x);
                data.vertices.push_back(position.y);
                data.vertices.push_back(position.z);

                // Parse texture index
                f_element = strtok_s(NULL, "/", &f_remainder);
                int vt_index = atoi(f_element) - 1;

                Vector2& texture_coord = data.texture_coords[vt_index];
                data.vertices.push_back(texture_coord.u);
                data.vertices.push_back(texture_coord.v);

                // Parse normal index
                f_element = strtok_s(NULL, "/", &f_remainder);
                int vn_index = atoi(f_element) - 1;

                Vector3& normal = data.normals[vn_index];
                data.vertices.push_back(normal.x);
                data.vertices.push_back(normal.y);
                data.vertices.push_back(normal.z);
            }

            // Log index
            indices.push_back(index);

            // Read next element in the face
            token = strtok_s(NULL, " ", &remainder);
        }

        assert(indices.size() >= 3);

        // We now parse the indices of the face into individual triangles.
        // If we have an N-gon formed by indices 0, 1, 2, 3, 4, 5,
        // we can triangulate it as [0,1,2], [0,2,3], [0,3,4], [0,4,5].
        for (int i = 2; i < indices.size(); i++)
        {
            data.indices.push_back(indices[0]);
            data.indices.push_back(indices[i - 1]);
            data.indices.push_back(indices[i - 2]);
        }

    }
}
}