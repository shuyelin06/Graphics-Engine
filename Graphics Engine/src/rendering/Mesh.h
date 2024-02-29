#pragma once

#include "VisualEngine.h"

#include <vector>
#include <string>

namespace Engine
{
namespace Graphics
{
	using namespace std;

	// Supported input data types
	enum InputData { XYZ_Position, RGB_Color };
	
	// Mesh Class
	// Stores information regarding vertices
	// and their index groupings to form a mesh.
	class Mesh
	{
	private:
		// Vertices and their data format
		vector<InputData> input_format;
		vector<vector<float>> vertex_list;

		// Index groupings
		vector<int> index_list;
	
	private:
		static int InputDataSize(InputData datatype);

	public:
		// Creates a mesh from a PLY file
		static VertexBuffer parsePLYFile(VisualEngine* graphics_engine, string ply_file);

	};

}
}