// #include "rendering/Mesh.h"

// Includes for Debug
#include <assert.h>
#include <windows.h>

// Includes for file parsing
#include <iostream>
#include <fstream>

#include <regex>
#include <vector>

// Contains the code necessary to parse a PLY file
// into a mesh. Separated from the rest of the Mesh.cpp
// implementation due to its inherent complexity

namespace Engine
{
namespace Graphics
{
	/*
	// ParsePLYFile
	// A simple PLY file parser. Only allows the ASCII 1.0 
	// file format.
	void Mesh::parsePLYFile(std::string ply_file, std::string mesh_name)
	{
		// Create input stream from PLY file
		std::ifstream file_stream(ply_file);

		// Check for success
		if (!file_stream.is_open())
			assert(false);

		// Variables for file reading and parsing 
		std::string line;
		std::smatch match;

		// Number of vertices and faces to expect while file parsing
		int num_faces = 0, num_vertices = 0;
		char layout = 0;

		// Parse the initial "ply" header
		if (getline(file_stream, line), line != "ply")
			assert(false);

		// This parser only supports the ASCII 1.0 PLY variant
		if (getline(file_stream, line), line != "format ascii 1.0")
			assert(false);

		// Read header information for layout vertex / face count
		{
			// Expect and parse the vertex description first
			if (getline(file_stream, line),
				regex_search(line, match, std::regex("element vertex (\\d+)")))
			{
				// Update number of vertices
				num_vertices = stoi(match.str(1));

				// Parse all vertex properties
				std::string properties("");
				std::regex re_property = std::regex("property float32 ([a-z]+)");

				// Read all properties
				while (getline(file_stream, line),
					regex_search(line, match, re_property))
				{
					// Add to properties string
					properties.append(match.str(1));
				}

				// Parse the properties string to see the intended layout. Expected order:
				// Position, RGB Color, Normals
				if (properties.substr(0, 3) == "xyz")
				{
					// Update layout
					layout |= XYZ;
					// Strip xyz from string
					properties = properties.substr(3, std::string::npos);
				}

				if (properties.substr(0, 3) == "rgb")
				{
					layout |= RGB;
					properties = properties.substr(3, std::string::npos);
				}

				if (properties.substr(0, 6) == "xnynzn")
				{
					layout |= NORMAL;
					properties = properties.substr(6, std::string::npos);
				}
			}
			else
				assert(false);

			// Expect and parse the face description next
			if (regex_search(line, match, std::regex("element face (\\d+)")))
			{
				num_faces = stoi(match.str(1));
			}
			else
				assert(false);

			// Skip to end of header
			while (line != "end_header")
				getline(file_stream, line);
		}

		// Mesh object
		meshes[mesh_name] = Mesh(layout);
		Mesh& mesh = meshes[mesh_name];

		// Read vertices
		int size = Mesh::VertexLayoutSize(layout);
		float* vertex = new float[size];

		{
			std::regex re_float("-?\\d+(\\.?\\d+)?(e-?\\d+)?");

			for (int i = 0; i < num_vertices; i++)
			{
				// Read vertex line
				getline(file_stream, line);

				// Match vertex format and populate vertices_list
				int vertex_i = 0;

				// Move through all float matches
				std::sregex_iterator iter = std::sregex_iterator(line.begin(), line.end(), re_float);

				while (iter != std::sregex_iterator() && vertex_i < size)
				{
					std::smatch match = *iter;
					// Add float to corresponding vertex property position
					vertex[vertex_i] = stof(match.str(0));
					vertex_i++;

					++iter;
				}

				// If there are remaining fields, throw an error
				if (vertex_i < size)
					assert(false);

				mesh.addVertex(vertex);
			}
		}

		delete[] vertex;

		// Read faces
		{
			std::regex face_format("3 (\\d+) (\\d+) (\\d+)");
			std::regex re_index("\\d+");

			for (int i = 0; i < num_faces; i++)
			{
				getline(file_stream, line);

				// Match vertex format and populate vertices_list
				int index_i = 0;

				int* indices = nullptr;
				int size = -1;

				// Move through all float matches
				std::sregex_iterator iter = std::sregex_iterator(line.begin(), line.end(), re_index);

				while (iter != std::sregex_iterator())
				{
					std::smatch match = *iter;

					if (index_i == 0)
					{
						size = stoi(match.str(0));

						assert(size >= 3);
						indices = new int[size];
					}
					else 
					{
						indices[index_i - 1] = stoi(match.str(0));
					}

					index_i++;
					++iter;
				}

				assert(size != -1);

				// Form triangles with these indices
				// If we have indices 0, 1, 2, 3,
				// then we form triangles 0, 1, 2 & 0, 2, 3
				for (int i = 2; i < size; i++)
				{
					mesh.addIndex(indices[0]);
					mesh.addIndex(indices[i - 1]);
					mesh.addIndex(indices[i]);
				}
			}
		}

	}
	*/
}
}