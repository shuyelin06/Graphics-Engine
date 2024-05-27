#include "rendering/Mesh.h"

// File I/O
#include <fstream>
#include <iostream>

#include <stdio.h>

// File Parsing
#include <map>
#include <vector>
#include <string.h>

// Debug
#include <assert.h>

#define SUCCESS true
#define FAILED false

namespace Engine
{
namespace Graphics
{
    // Stores 3-component vectors (position, normal)
    struct OBJVector3 {
        float x, y, z;
    };

    // Stores 2-component vectors (texture coordinates)
    struct OBJVector2 {
        float u, v;
    };

    struct OBJData
    {
        // Indexed data
        std::vector<OBJVector3> positions;
        std::vector<OBJVector2> texture_coords;
        std::vector<OBJVector3> normals;

        // Maps vertex index data -> a single unique vertex index
        std::vector<float> vertices;
        std::vector<int> indices;

        std::map<std::string, int> vertex_map;
    };

    // Helper Parsers
    static bool Parse_V(char* line, OBJData& data);
    static bool Parse_VT(char* line, OBJData& data);
    static bool Parse_VN(char* line, OBJData& data);
    static void Parse_F(char* line, OBJData& data);

    // https://www.reddit.com/r/opengl/comments/qs4wdi/parsing_an_obj_file_for_use_with_an_index_buffer/
    
    // ParseOBJFile:
    // Parses an OBJ file to generate a renderable mesh
	void Mesh::ParseOBJFile(std::string obj_file)
	{
        // Open target file with file reader
        std::ifstream file(obj_file);

        // Stores lines to be read from the reader
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

            // Vertex Data (v): Parse x,y,z Position
            if (strcmp(token, "v") == 0)
                Parse_V(remainder, data);
            else if (strcmp(token, "vt") == 0)
                Parse_VT(remainder, data);
            else if (strcmp(token, "vn") == 0)
                Parse_VN(remainder, data);
            else if (strcmp(token, "f") == 0)
                Parse_F(remainder, data);
            else {
                std::cout << token << "\n";
                std::cout << remainder << "\n";
            }

        }

        // Create mesh
        meshes["Panda"] = Mesh(XYZ | NORMAL);
        Mesh& mesh = meshes["Panda"];

        mesh.vertices = data.vertices;
        mesh.indices = data.indices;
        mesh.setShaders("Default", "Default");

        file.close();
    }

    // Parses a "v" data element for an x, y, z position.
    static bool Parse_V(char* line, OBJData& data)
    {
        char* token = NULL;
        char* remainder = NULL;

        OBJVector3 vector;

        // Parse X
        std::cout << "v" << "\n";

        token = strtok_s(line, " ", &remainder);
        std::cout << token << "\n";
        if (token == NULL)
            return FAILED;

        vector.x = atof(token);

        // Parse Y
        token = strtok_s(NULL, " ", &remainder);
        std::cout << token << "\n";
        if (token == NULL)
            return FAILED;

        vector.y = atof(token);

        // Parse Z
        token = strtok_s(NULL, " ", &remainder);
        std::cout << token << "\n";
        if (token == NULL)
            return FAILED;

        vector.z = atof(token);

        // Check that there is nothing else left
        token = strtok_s(NULL, "", &remainder);
        if (token != NULL)
            return FAILED;

        // Push to vector
        data.positions.push_back(vector);

        return SUCCESS;
    }

    // Parses a "vt" data element for a u, v texture coordinate
    static bool Parse_VT(char* line, OBJData& data)
    {
        OBJVector2 texture_coord;

        char* token = NULL;
        char* remainder = NULL;

        // Parse u
        token = strtok_s(line, " ", &remainder);

        if (token == NULL)
            return FAILED;

        texture_coord.u = atof(token);

        // Parse v
        token = strtok_s(NULL, " ", &remainder);

        if (token == NULL)
            return FAILED;

        texture_coord.v = atof(token);

        // Check that there is nothing else left
        token = strtok_s(NULL, "", &remainder);

        if (token != NULL)
            return FAILED;

        // Push to vector
        data.texture_coords.push_back(texture_coord);

        return SUCCESS;
    }

    // Parses a "vn" data element for a x, y, z normal vector
    static bool Parse_VN(char* line, OBJData& data)
    {
        char* token = NULL;
        char* remainder = NULL;

        OBJVector3 vector;

        // Parse X
        token = strtok_s(line, " ", &remainder);

        if (token == NULL)
            return FAILED;

        vector.x = atof(token);

        // Parse Y
        token = strtok_s(NULL, " ", &remainder);

        if (token == NULL)
            return FAILED;

        vector.y = atof(token);

        // Parse Z
        token = strtok_s(NULL, " ", &remainder);

        if (token == NULL)
            return FAILED;

        vector.z = atof(token);

        // Check that there is nothing else left
        token = strtok_s(NULL, "", &remainder);

        if (token != NULL)
            return FAILED;

        // Push to vector
        data.normals.push_back(vector);

        return SUCCESS;
    }

    void Parse_F(char* line, OBJData& data)
    {
        char* token = NULL;
        char* remainder = NULL;

        // List of indices for the face
        std::vector<int> indices;

        // Parse each element of the face
        token = strtok_s(line, " ", &remainder);

        int index_id = data.indices.size();

        while (token != NULL)
        {
            int index;

            // Check if this vertex combination exists. If it does
            // not, add it to the vertex_map
            if (data.vertex_map.contains(token))
                index = data.vertex_map[token];
            else {
                index = index_id;
                index_id++;
                data.vertex_map[token] = index;

                char* f_element = NULL;
                char* f_remainder = NULL;

                // Parse first element, position
                f_element = strtok_s(token, "/", &f_remainder);
                int v_index = atoi(f_element) - 1;

                data.vertices.push_back(data.positions[v_index].x);
                data.vertices.push_back(data.positions[v_index].y);
                data.vertices.push_back(data.positions[v_index].z);

                // Parse second element, texture
                f_element = strtok_s(NULL, "/", &f_remainder);
                int vt_index = atoi(f_element) - 1;

                // TODO
                // data.vertices.push_back(data.positions[v_index].x);

                // Parse third element, normal
                f_element = strtok_s(NULL, "/", &f_remainder);
                int vn_index = atoi(f_element) - 1;

                data.vertices.push_back(data.normals[vn_index].x);
                data.vertices.push_back(data.normals[vn_index].y);
                data.vertices.push_back(data.normals[vn_index].z);
            }

            indices.push_back(index);

            token = strtok_s(NULL, " ", &remainder);
        }

        // Parse indices of face into individual triangles
        for (int i = 2; i < indices.size(); i++)
        {
           // TODO: Need to generate a triangulation as some of these are given 
            // as n-gons :(
            
            data.indices.push_back(indices[0]);

            data.indices.push_back(indices[i - 1]);
            
            

           
            data.indices.push_back(indices[i]);
        }

    }
}
}