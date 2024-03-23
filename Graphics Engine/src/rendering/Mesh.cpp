#include "Mesh.h"

// Includes for Debug
#include <assert.h>
#include <windows.h>

// Includes for file parsing
#include <iostream>
#include <fstream>

#include <regex>
#include <vector>

namespace Engine
{
namespace Graphics
{
	// VertexLayoutSize
	// Static method returning the number of floats a given 
	// vertex layout has
	int Mesh::VertexLayoutSize(char layout)
	{
		int size = ((1 & layout) * 3)	// 1st Bit: XYZ Position
			+ ((1 & (layout >> 1)) * 3)	// 2nd Bit: RGB Color
			+ ((1 & (layout >> 2)) * 3); // 3rd Bit: XYZ Normal
		return size;
	}

	// GenerateVertexLayout
	// Static method that lets us create a vertex layout, given
	// the input format 
	char Mesh::GenerateVertexLayout(bool pos, bool rgb, bool norm)
	{
		char layout = (pos & 1)		// 1st Bit: XYZ Position
					| ((rgb & 1) << 1) // 2nd Bit: RGB Color
					| ((norm & 1) << 2); // 3rd Bit: XYZ Normal
		return layout;
	}

	// Mesh Constructors
	// Default Constructor
	Mesh::Mesh()
	{
		vertex_layout = 0;
	}

	// Creates an empty mesh with a specified data layout
	Mesh::Mesh(char layout) 
	{
		// Save vertex layout
		vertex_layout = layout;

		// Reserve space for 3 vertices
		vertices.reserve(VertexLayoutSize(layout) * 3);
		indices.reserve(3);
	}

	// AddVertex:
	// Adds a vertex to the Mesh's list of vertices.
	// Assumes the array of floats is the same size as the
	// vertex_layout provided
	void Mesh::addVertex(float vertex[]) 
	{
		int size = VertexLayoutSize(vertex_layout);

		for (int i = 0; i < size; i++) {
			vertices.push_back(vertex[i]);
		}
	}

	// AddIndex:
	// Adds an index to the Mesh's list of indices
	// (specifying vertex groups to be rendered).
	void Mesh::addIndex(int index)
	{
		indices.push_back(index);
	}
	
	char Mesh::getLayout()
	{
		return vertex_layout;
	}

	const vector<float> Mesh::getVertexBuffer()
	{
		return vertices;
	}
	
	const vector<int> Mesh::getIndexBuffer()
	{
		return indices;
	}

	// ParsePLYFile
	// A simple PLY file parser. Assumes ASCII format.
	Mesh Mesh::parsePLYFile(string ply_file, char layout)
	{
		// Mesh object
		Mesh mesh = Mesh(layout);

		// Create input stream from PLY file
		std::ifstream infile(ply_file);

		// If successful, read file line by line
		if (infile.is_open())
		{
			// Will store the input line
			std::string line;
			std::smatch match;

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
					regex re_vertex_element("element vertex (\\d+)");
					regex re_face_element("element face (\\d+)");

					// Get the next line
					getline(infile, line);

					// Parse intermediate lines 
					while (!regex_search(line, match, re_vertex_element))
						getline(infile, line);

					// Parse the vertex element
					if (regex_search(line, match, re_vertex_element))
						num_vertices = stoi(match.str(1));
					else
						assert(false);

					// Parse intermediate lines
					while (!regex_search(line, match, re_face_element))
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
				int size = VertexLayoutSize(layout);
				float* vertex = new float[size];
				regex re_float("-?\\d+(\\.?\\d+)?");

				for (int i = 0; i < num_vertices; i++)
				{
					// Read vertex line
					getline(infile, line);

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

					// HACK - Populate the remaining fields that don't exist
					while (vertex_i < size)
					{
						vertex[vertex_i++] = (((float) rand() / (RAND_MAX)));
					}

					mesh.addVertex(vertex);
				}

				delete[] vertex;
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
						mesh.addIndex(stoi(match.str(1)));
						mesh.addIndex(stoi(match.str(2)));
						mesh.addIndex(stoi(match.str(3)));
					}
				}
			}

			
		}

		return mesh;
	}
}
}