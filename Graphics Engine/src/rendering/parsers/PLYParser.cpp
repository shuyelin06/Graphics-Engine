#include "rendering/Mesh.h"

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

using namespace std;

namespace Engine
{
namespace Graphics
{
	
	// ParsePLYFile
	// A simple PLY file parser. Only allows the ASCII 1.0 
	// file format.
	Mesh Mesh::parsePLYFile(string ply_file)
	{
		// Create input stream from PLY file
		ifstream file_stream(ply_file);

		// Check for success
		if (!file_stream.is_open())
			assert(false);

		// Variables for file reading and parsing 
		string line;
		smatch match;

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
				regex_search(line, match, regex("element vertex (\\d+)")))
			{
				// Update number of vertices
				num_vertices = stoi(match.str(1));

				// Parse all vertex properties
				string properties("");
				regex re_property = regex("property float32 ([a-z]+)");

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
					properties = properties.substr(3, string::npos);
				}

				if (properties.substr(0, 3) == "rgb")
				{
					layout |= RGB;
					properties = properties.substr(3, string::npos);
				}

				if (properties.substr(0, 6) == "xnynzn")
				{
					layout |= NORMAL;
					properties = properties.substr(6, string::npos);
				}
			}
			else
				assert(false);

			// Expect and parse the face description next
			if (regex_search(line, match, regex("element face (\\d+)")))
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
		Mesh mesh = Mesh(layout);

		// Read vertices
		{
			int size = Mesh::VertexLayoutSize(layout);
			float* vertex = new float[size];
			regex re_float("-?\\d+(\\.?\\d+)?");

			for (int i = 0; i < num_vertices; i++)
			{
				// Read vertex line
				getline(file_stream, line);

				// Match vertex format and populate vertices_list
				int vertex_i = 0;

				// Move through all float matches
				sregex_iterator iter = std::sregex_iterator(line.begin(), line.end(), re_float);

				while (iter != std::sregex_iterator())
				{
					std::smatch match = *iter;
					// Add float to corresponding vertex property position
					vertex[vertex_i++] = stof(match.str(0));

					++iter;
				}

				// If there are remaining fields, throw an error
				if (vertex_i < size)
					assert(false);

				mesh.addVertex(vertex);
			}

			delete[] vertex;
		}

		// Read faces
		{
			regex face_format("3 (\\d+) (\\d+) (\\d+)");

			for (int i = 0; i < num_faces; i++)
			{
				getline(file_stream, line);

				if (regex_search(line, match, face_format))
				{
					// Add indices to index list
					mesh.addIndex(stoi(match.str(1)));
					mesh.addIndex(stoi(match.str(2)));
					mesh.addIndex(stoi(match.str(3)));
				}
			}
		}

		return mesh;
	}

}
}