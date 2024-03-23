#pragma once

#include <vector>
#include <string>

namespace Engine
{
namespace Graphics
{
	using namespace std;

	// Mesh Class
	// Stores information regarding vertices
	// and their index groupings to form a mesh.
	class Mesh
	{
	private:
		// Stores Input DataTypes in Bits. From Right -> Left:
		// 1st Bit) Position (X,Y,Z)
		// 2nd Bit) Color (R,G,B)
		// 3rd Bit) Normal (X,Y,Z)
		char vertex_layout;
		
		// Vertex buffer and index buffer
		vector<float> vertices;
		vector<int> indices;
	
	public:
		// Number of floats a vertex_layout has
		static int VertexLayoutSize(char layout);
		static char GenerateVertexLayout(bool pos, bool rgb, bool norm);

		// Constructor
		Mesh();
		Mesh(char layout);

		// Add vertices or indices to the mesh
		void addVertex(float vertex[]);
		void addIndex(int index);

		// Getters for index and vertex buffers
		char getLayout();
		const vector<float> getVertexBuffer();
		const vector<int> getIndexBuffer();

		// File parsers that can generate meshes
		static Mesh parsePLYFile(string ply_file, char layout);

	};

}
}