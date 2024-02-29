#include "Parser.h"

// Includes for IO
#include <iostream>
#include <fstream>

// Includes for Debug
#include <assert.h>
#include <windows.h>

// Includes for Parsing
#include <regex>
#include <vector>   

#include "math/Vector3.h"

namespace Engine
{
namespace Utility
{
	// PLY Parsing
	// Parses a PLY file
	// Example format: https://people.sc.fsu.edu/~jburkardt/data/ply/ply.html
	void Parser::parsePLYFile(std::string ply_file)
	{
		// Create input stream from PLY file
		std::ifstream infile(ply_file);

		// If successful, read file line by line
		if (infile.is_open())
		{
			// Input line
			std::string line;

			int num_vertices;
			int num_faces;

			// Parse header of PLY file
			{
				// Regex matches for vertex and face property
				std::regex vertex_property("element vertex (\\d+)");
				std::regex face_property("element face (\\d+)");

				std::smatch match;

				while (getline(infile, line), line != "end_header")
				{
					if (regex_search(line, match, vertex_property))
					{
						num_vertices = std::stoi(match.str(1));
					}
					else if (regex_search(line, match, face_property))
					{
						num_faces = std::stoi(match.str(1));
					}
				}
			}
			
			// Parse indices
			for (int i = 0; i < num_vertices; i++)
			{

			}

		}
	}

}
}