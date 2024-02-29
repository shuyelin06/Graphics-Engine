#include "OBJParser.h"

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
using namespace Math;

namespace Utility
{

	void OBJParser::parseFile(std::string obj_file)
	{
        // Create input stream
        std::ifstream infile(obj_file);
        
        // If successful, read file line by line
        if (infile.is_open())
        {
            // Input line
            std::string line;

            // Vertices
            std::vector<Vector3> vertices;

            // Regex matches
            std::regex re_vertex("v (-?\\d+\.\\d+) (-?\\d+\.\\d+) (-?\\d+\.\\d+)");
            std::regex re_face("f (\\d+/\\d+/\\d+( \\d+/\\d+/\\d+)+)");

            std::smatch match;

            // Match line with potential parsing options
            while (getline(infile, line))
            {
                // New Vertex: "v_FLOAT_FLOAT_FLOAT" 
                if (regex_search(line, match, re_vertex)) 
                {
                    // Locate the 
                    float x = std::stof(match.str(1));
                    float y = std::stof(match.str(2));
                    float z = std::stof(match.str(3));

                    vertices.push_back(Vector3(x, y, z));

                    OutputDebugStringA("Vertex Match!");

                    OutputDebugStringA(match.str(1).c_str());
                    OutputDebugStringA(match.str(2).c_str());
                    OutputDebugStringA(match.str(3).c_str());

                    OutputDebugStringA("\n");
                }
                // New Face: "f INT/INT/INT INT/INT/INT ..."
                else if (regex_search(line, match, re_face)) 
                {
                    std::string indices = match.str(1);
                    
                    std::regex re_parse(" ?(\\d+)/(\\d+)/(\\d+)(( \\d+/\\d+/\\d+)*)");
                    std::smatch parse_match;

                    while (regex_search(indices, parse_match, re_parse))
                    {
                        OutputDebugStringA("---\n");
                        OutputDebugStringA(parse_match.str(1).c_str());
                        OutputDebugStringA("\n");
                        OutputDebugStringA(parse_match.str(2).c_str());
                        OutputDebugStringA("\n");
                        OutputDebugStringA(parse_match.str(3).c_str());
                        OutputDebugStringA("\n");

                        // Get first, second, and third indices
                        int v_index = std::stoi(parse_match.str(1));
                        int vt_index = std::stoi(parse_match.str(2));
                        int vn_index = std::stoi(parse_match.str(3));

                        // Update indices
                        indices = parse_match.str(4);

                        // TODO: Load indices into vertex buffer, and create a mesh object.
                    }
                    
                }
            }

            // Done reading; close file
            infile.close();
        }
        // Else, fail
        else
        {
            OutputDebugStringA("Parsing failed!\n");
            assert(false);
        }
	}
}
}