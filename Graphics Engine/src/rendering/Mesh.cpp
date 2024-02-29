#include "Mesh.h"

// Includes for Debug
#include <assert.h>
#include <windows.h>

// Includes for file parsing
#include <iostream>
#include <fstream>

#include <regex>
#include <vector>   

#include "objects/Renderable.h"

namespace Engine
{
namespace Graphics
{
	// Input_Data_Size
	// Returns the size (number of floats) for a 
	// given data type
	int Mesh::InputDataSize(InputData datatype) {
		int size = 0;

		switch (datatype) 
		{
		case XYZ_Position:
			size = 3;
			break;

		case RGB_Color:
			size = 3;
			break;
		}

		return size;
	}

	// ParsePLYFile
	// A simple PLY file parser. Assumes ASCII format.
	enum PLY_Vertex_Property { X, Y, Z, R, G, B };
	VertexBuffer Mesh::parsePLYFile(VisualEngine* graphics_engine, string ply_file)
	{
		// Create input stream from PLY file
		std::ifstream infile(ply_file);

		// If successful, read file line by line
		if (infile.is_open())
		{
			// Mesh object
			Mesh mesh;

			// Will store the input line
			std::string line;
			std::smatch match;

			// Format of vertices in the PLY file
			vector<PLY_Vertex_Property> vertex_format;

			int num_vertices = 0;
			int num_faces = 0;

			// Read header information
			{
				// Parse the initial "ply"
				if (getline(infile, line), line != "ply")
					assert(false);
				else
				{
					// Parse the format of the PLY file; only supports ASCII 1.0
					if (getline(infile, line), line != "format ascii 1.0")
						assert(false);

					// Define regex to parse portions of the header
					regex re_comment("comment .*");

					regex re_vertex_element("element vertex (\\d+)");
					regex re_vertex_property("property ([a-z0-9]+) ([a-z]+)");
					regex re_face_element("element face (\\d+)");

					// Get the next line
					getline(infile, line);

					// Parse intermediate comments
					while (regex_search(line, match, re_comment))
						getline(infile, line);

					// Parse the vertex element
					if (regex_search(line, match, re_vertex_element))
					{
						// Save number of vertices
						num_vertices = stoi(match.str(1));

						// Parse properties of the vertices
						getline(infile, line);

						while (regex_search(line, match, re_vertex_property))
						{
							string prop = match.str(2);

							if (prop == "x")
								vertex_format.push_back(X);
							else if (prop == "y")
								vertex_format.push_back(Y);
							else if (prop == "z")
								vertex_format.push_back(Z);
							else if (prop == "r")
								vertex_format.push_back(R);
							else if (prop == "g")
								vertex_format.push_back(G);
							else if (prop == "b")
								vertex_format.push_back(B);
							else
								assert(false);

							getline(infile, line);
						}
					}
					else
						assert(false);

					// Parse intermediate comments
					while (regex_search(line, match, re_comment))
						getline(infile, line);

					// Parse the face element
					if (regex_search(line, match, re_face_element))
						num_faces = stoi(match.str(1));
					else
						assert(false);

					// Skip to end of header
					while (line != "end_header")
						getline(infile, line);
				}
			}

			// Read vertices
			{
				regex re_float("-?\\d+(\\.?\\d+)?");

				for (int i = 0; i < num_vertices; i++)
				{
					// Read vertex line
					getline(infile, line);

					// Match vertex format and populate vertices_list
					vector<float> vertex;
					vertex.resize(vertex_format.size());

					// Move through all float matches
					int j = 0;

					for (sregex_iterator iter = std::sregex_iterator(line.begin(), line.end(), re_float);
						iter != std::sregex_iterator();
						++iter, j++)
					{
						// Add float to corresponding vertex property position
						std::smatch match = *iter;
						vertex[vertex_format[j]] = stof(match.str(0));
					}

					// Populate the remaining fields that don't exist
					while (j < vertex_format.size())
					{
						vertex[j] = (((float)rand() / (RAND_MAX)));
						j++;
					}

					mesh.vertex_list.push_back(vertex);
				}
			}

			// Read faces
			{
				regex face_format("3 (\\d+) (\\d+) (\\d+)");

				for (int i = 0; i < num_faces; i++)
				{
					getline(infile, line);

					if (regex_search(line, match, face_format))
					{
						// Add indices to index list
						mesh.index_list.push_back(stoi(match.str(1)));
						mesh.index_list.push_back(stoi(match.str(2)));
						mesh.index_list.push_back(stoi(match.str(3)));
					}
				}
			}

			// Create renderable vertex buffer
			float* vertices = new float[mesh.index_list.size() * vertex_format.size()];

			for (int i = 0; i < mesh.index_list.size(); i++)
			{
				vector vertex = mesh.vertex_list[mesh.index_list[i]];

				for (int j = 0; j < vertex_format.size(); j++)
				{
					vertices[i * vertex_format.size() + j] = vertex[j];
				}
			}

			// Create vertex buffer
			return graphics_engine->generate_vertex_buffer(vertices, vertex_format.size(), mesh.index_list.size());
		}

		return Renderable::getCubeMesh(graphics_engine);
	}
}
}